"""
Quick Performance Test for TexDit Architectures
Run this to get a fast comparison between current and single LLM approaches
"""

import requests
import time
import json
from concurrent.futures import ThreadPoolExecutor
import subprocess
import os
import signal

class QuickArchitectureTest:
    def __init__(self):
        self.current_server_url = "http://localhost:5000"
        self.single_llm_server_url = "http://localhost:5001"
        self.test_texts = {
            "short": "The quick brown fox jumps over the lazy dog. This is a simple test.",
            "medium": """Artificial intelligence has revolutionized many aspects of modern life, from healthcare to transportation. 
            Machine learning algorithms can now diagnose diseases, predict weather patterns, and even create art. 
            However, with these advancements come new challenges and ethical considerations.""",
            "long": """The field of artificial intelligence has undergone remarkable transformations over the past decade, 
            fundamentally altering how we approach complex problems across numerous domains. From healthcare systems 
            that can predict patient outcomes with unprecedented accuracy to autonomous vehicles navigating busy city streets, 
            AI technologies have moved from the realm of science fiction into practical, everyday applications. 
            Machine learning algorithms, particularly deep learning models, have demonstrated exceptional capabilities 
            in pattern recognition, natural language processing, and decision-making tasks. These systems can now 
            analyze vast amounts of data in seconds, identifying trends and insights that would take human analysts 
            weeks or months to discover."""
        }
        
    def test_server_health(self, server_url, timeout=5):
        """Test if server is running and healthy"""
        try:
            response = requests.get(f"{server_url}/health", timeout=timeout)
            return response.status_code == 200
        except:
            return False
    
    def test_endpoint(self, server_url, endpoint, data, timeout=30):
        """Test a specific endpoint and measure performance"""
        start_time = time.time()
        
        try:
            response = requests.post(
                f"{server_url}/api/{endpoint}",
                json=data,
                timeout=timeout
            )
            
            end_time = time.time()
            
            if response.status_code == 200:
                result = response.json()
                return {
                    "success": True,
                    "time_seconds": round(end_time - start_time, 3),
                    "response": result,
                    "error": None
                }
            else:
                return {
                    "success": False,
                    "time_seconds": round(end_time - start_time, 3),
                    "response": None,
                    "error": f"HTTP {response.status_code}: {response.text}"
                }
                
        except Exception as e:
            end_time = time.time()
            return {
                "success": False,
                "time_seconds": round(end_time - start_time, 3),
                "response": None,
                "error": str(e)
            }
    
    def run_comparison(self):
        """Run a comprehensive comparison between both architectures"""
        print("🚀 TexDit Architecture Quick Performance Test")
        print("=" * 50)
        
        # Check server availability
        print("\n🔍 Checking server availability...")
        current_available = self.test_server_health(self.current_server_url)
        single_llm_available = self.test_server_health(self.single_llm_server_url)
        
        print(f"  Current Architecture (port 5000): {'✅' if current_available else '❌'}")
        print(f"  Single LLM Architecture (port 5001): {'✅' if single_llm_available else '❌'}")
        
        if not current_available and not single_llm_available:
            print("\n❌ No servers are running. Please start at least one server.")
            print("  Current: python backend/server.py")
            print("  Single LLM: python backend/single_llm_server.py")
            return
        
        # Test endpoints
        endpoints = ["summarise", "keywords", "tone", "rephrase"]
        results = {
            "current_architecture": {},
            "single_llm_architecture": {},
            "comparison": {}
        }
        
        for endpoint in endpoints:
            print(f"\n🧪 Testing {endpoint}...")
            
            for text_size, text in self.test_texts.items():
                print(f"  📝 {text_size} text ({len(text.split())} words)...")
                
                # Prepare test data
                test_data = {"text": text}
                if endpoint == "summarise":
                    test_data["ratio"] = 0.25
                
                # Test current architecture
                if current_available:
                    current_result = self.test_endpoint(self.current_server_url, endpoint, test_data)
                    if endpoint not in results["current_architecture"]:
                        results["current_architecture"][endpoint] = {}
                    results["current_architecture"][endpoint][text_size] = current_result
                    
                    status = "✅" if current_result["success"] else "❌"
                    time_str = f"{current_result['time_seconds']}s"
                    print(f"    Current: {status} {time_str}")
                
                # Test single LLM architecture
                if single_llm_available:
                    single_result = self.test_endpoint(self.single_llm_server_url, endpoint, test_data)
                    if endpoint not in results["single_llm_architecture"]:
                        results["single_llm_architecture"][endpoint] = {}
                    results["single_llm_architecture"][endpoint][text_size] = single_result
                    
                    status = "✅" if single_result["success"] else "❌"
                    time_str = f"{single_result['time_seconds']}s"
                    print(f"    Single LLM: {status} {time_str}")
                
                # Compare if both available
                if current_available and single_llm_available:
                    if current_result["success"] and single_result["success"]:
                        speed_diff = single_result["time_seconds"] - current_result["time_seconds"]
                        if speed_diff > 0:
                            print(f"    📊 Current is {speed_diff:.2f}s faster")
                        else:
                            print(f"    📊 Single LLM is {abs(speed_diff):.2f}s faster")
        
        # Generate summary
        self.print_summary(results, current_available, single_llm_available)
        
        # Save detailed results
        filename = f"quick_test_results_{int(time.time())}.json"
        with open(filename, 'w') as f:
            json.dump(results, f, indent=2)
        print(f"\n📁 Detailed results saved to: {filename}")
        
        return results
    
    def print_summary(self, results, current_available, single_llm_available):
        """Print a summary of the test results"""
        print("\n" + "=" * 60)
        print("📊 QUICK TEST SUMMARY")
        print("=" * 60)
        
        if current_available:
            print("\n🏗️ CURRENT ARCHITECTURE (DistilBART + Rules)")
            self.print_architecture_summary(results["current_architecture"])
        
        if single_llm_available:
            print("\n🤖 SINGLE LLM ARCHITECTURE")
            self.print_architecture_summary(results["single_llm_architecture"])
        
        if current_available and single_llm_available:
            print("\n⚖️ PERFORMANCE COMPARISON")
            self.print_comparison(results)
        
        print("\n💡 RECOMMENDATIONS:")
        print("  1. Target: < 3 seconds per query for good UX")
        print("  2. Consider quality vs speed trade-offs")
        print("  3. Test with larger models if speed is acceptable")
        print("  4. Consider caching for repeated operations")
    
    def print_architecture_summary(self, arch_results):
        """Print summary for one architecture"""
        for endpoint, endpoint_results in arch_results.items():
            successful_times = []
            failed_count = 0
            
            for text_size, result in endpoint_results.items():
                if result["success"]:
                    successful_times.append(result["time_seconds"])
                else:
                    failed_count += 1
            
            if successful_times:
                avg_time = sum(successful_times) / len(successful_times)
                min_time = min(successful_times)
                max_time = max(successful_times)
                print(f"  {endpoint}: {avg_time:.2f}s avg ({min_time:.2f}s-{max_time:.2f}s)")
            
            if failed_count > 0:
                print(f"    ❌ {failed_count} failures")
    
    def print_comparison(self, results):
        """Print detailed comparison between architectures"""
        current = results["current_architecture"]
        single_llm = results["single_llm_architecture"]
        
        for endpoint in current.keys():
            if endpoint in single_llm:
                print(f"\n  📈 {endpoint.upper()}:")
                
                current_times = [r["time_seconds"] for r in current[endpoint].values() if r["success"]]
                single_times = [r["time_seconds"] for r in single_llm[endpoint].values() if r["success"]]
                
                if current_times and single_times:
                    current_avg = sum(current_times) / len(current_times)
                    single_avg = sum(single_times) / len(single_times)
                    
                    if current_avg < single_avg:
                        diff = single_avg - current_avg
                        print(f"    ✅ Current is {diff:.2f}s faster on average")
                    else:
                        diff = current_avg - single_avg
                        print(f"    🤖 Single LLM is {diff:.2f}s faster on average")
                    
                    print(f"    Current: {current_avg:.2f}s | Single LLM: {single_avg:.2f}s")

def main():
    tester = QuickArchitectureTest()
    tester.run_comparison()

if __name__ == "__main__":
    main()
