"""
TexDit Server Manager
Utility to start and manage both architecture test servers
"""

import subprocess
import time
import signal
import os
import sys
import threading
import requests
from pathlib import Path

class ServerManager:
    def __init__(self):
        self.current_server_process = None
        self.single_llm_server_process = None
        self.backend_dir = Path(__file__).parent
        
    def check_port(self, port, timeout=5):
        """Check if a port is available"""
        try:
            response = requests.get(f"http://localhost:{port}/health", timeout=timeout)
            return response.status_code == 200
        except:
            return False
    
    def start_current_server(self):
        """Start the current DistilBART server"""
        print("🚀 Starting Current Architecture Server (DistilBART + Rules)...")
        
        if self.check_port(5000):
            print("  ✅ Server already running on port 5000")
            return True
        
        try:
            # Change to backend directory
            server_path = self.backend_dir / "server.py"
            
            self.current_server_process = subprocess.Popen(
                [sys.executable, str(server_path)],
                cwd=str(self.backend_dir),
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True
            )
            
            # Wait for server to start
            for i in range(30):  # Wait up to 30 seconds
                if self.check_port(5000):
                    print(f"  ✅ Current server started successfully on port 5000")
                    return True
                time.sleep(1)
                print(f"  ⏳ Waiting for server to start... ({i+1}/30)")
            
            print("  ❌ Current server failed to start within 30 seconds")
            return False
            
        except Exception as e:
            print(f"  ❌ Failed to start current server: {e}")
            return False
    
    def start_single_llm_server(self):
        """Start the single LLM server"""
        print("🤖 Starting Single LLM Architecture Server...")
        
        if self.check_port(5001):
            print("  ✅ Server already running on port 5001")
            return True
        
        try:
            server_path = self.backend_dir / "single_llm_server.py"
            
            self.single_llm_server_process = subprocess.Popen(
                [sys.executable, str(server_path)],
                cwd=str(self.backend_dir),
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True
            )
            
            # Wait for server to start (longer timeout as model loading takes time)
            for i in range(60):  # Wait up to 60 seconds for model loading
                if self.check_port(5001):
                    print(f"  ✅ Single LLM server started successfully on port 5001")
                    return True
                time.sleep(1)
                print(f"  ⏳ Loading model and starting server... ({i+1}/60)")
            
            print("  ❌ Single LLM server failed to start within 60 seconds")
            return False
            
        except Exception as e:
            print(f"  ❌ Failed to start single LLM server: {e}")
            return False
    
    def stop_servers(self):
        """Stop both servers"""
        print("\n🛑 Stopping servers...")
        
        if self.current_server_process:
            try:
                self.current_server_process.terminate()
                self.current_server_process.wait(timeout=10)
                print("  ✅ Current server stopped")
            except:
                try:
                    self.current_server_process.kill()
                    print("  ⚡ Current server force killed")
                except:
                    print("  ❌ Failed to stop current server")
        
        if self.single_llm_server_process:
            try:
                self.single_llm_server_process.terminate()
                self.single_llm_server_process.wait(timeout=10)
                print("  ✅ Single LLM server stopped")
            except:
                try:
                    self.single_llm_server_process.kill()
                    print("  ⚡ Single LLM server force killed")
                except:
                    print("  ❌ Failed to stop single LLM server")
    
    def run_comparison_test(self):
        """Run the quick comparison test"""
        print("\n🧪 Running Architecture Comparison Test...")
        
        try:
            test_script = self.backend_dir / "quick_test.py"
            result = subprocess.run(
                [sys.executable, str(test_script)],
                cwd=str(self.backend_dir),
                capture_output=False,  # Show output in real-time
                text=True
            )
            
            if result.returncode == 0:
                print("\n✅ Comparison test completed successfully!")
            else:
                print(f"\n❌ Comparison test failed with return code: {result.returncode}")
                
        except Exception as e:
            print(f"\n❌ Failed to run comparison test: {e}")

def main():
    print("=" * 60)
    print("🏗️ TexDit Architecture Testing Server Manager")
    print("=" * 60)
    
    manager = ServerManager()
    
    try:
        print("\n1️⃣ PHASE 1: Starting Current Architecture Server")
        current_started = manager.start_current_server()
        
        print("\n2️⃣ PHASE 2: Starting Single LLM Architecture Server")
        single_started = manager.start_single_llm_server()
        
        if not current_started and not single_started:
            print("\n❌ No servers could be started. Exiting.")
            return
        
        print("\n3️⃣ PHASE 3: Running Performance Comparison")
        if current_started or single_started:
            manager.run_comparison_test()
        
        print("\n" + "=" * 60)
        print("🎯 TEST COMPLETE!")
        print("=" * 60)
        
        if current_started and single_started:
            print("✅ Both architectures tested successfully")
            print("📊 Check the results above and the generated JSON file")
        elif current_started:
            print("⚠️  Only current architecture tested (single LLM failed)")
        elif single_started:
            print("⚠️  Only single LLM architecture tested (current failed)")
        
        print("\n💡 Next Steps:")
        print("  1. Analyze the performance metrics above")
        print("  2. Consider user experience (< 3s per query ideal)")
        print("  3. Test quality of outputs manually")
        print("  4. Decide on architecture based on speed vs quality")
        
        input("\n⏸️  Press Enter to stop servers and exit...")
        
    except KeyboardInterrupt:
        print("\n⚠️  Interrupted by user")
    
    finally:
        manager.stop_servers()
        print("👋 Goodbye!")

if __name__ == "__main__":
    main()
