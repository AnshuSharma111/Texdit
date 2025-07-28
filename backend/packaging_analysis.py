"""
TexDit Packaging Size vs Performance Analysis
Compare realistic deployment scenarios for offline app packaging

This tests the actual trade-off you're facing:
- Multiple specialized models (6-7GB total) vs Single LLM (3-7GB)
- Focus on packaging size and user experience
"""

import time
import requests
import json
from datetime import datetime

class PackagingSizeAnalysis:
    def __init__(self):
        self.current_server_url = "http://localhost:5000"
        self.single_llm_server_url = "http://localhost:5001"
        
        # Realistic model sizes for offline packaging
        self.specialized_models = {
            "summarization": {"model": "DistilBART-CNN-12-6", "size_gb": 1.14},
            "keywords": {"model": "ml6team/keyphrase-extraction-distilbert", "size_gb": 0.5},
            "grammar": {"model": "prithivida/grammar_error_correcter_v1", "size_gb": 1.2},
            "ner": {"model": "dslim/bert-base-NER", "size_gb": 0.4},
            "tone": {"model": "cardiffnlp/twitter-roberta-base-sentiment", "size_gb": 0.5},
            "rephrase": {"model": "tuner007/pegasus_paraphrase", "size_gb": 2.3}
        }
        
        self.single_llm_options = {
            "Flan-T5-Large": {"size_gb": 3.0, "expected_speed": "medium"},
            "Phi-3-Mini": {"size_gb": 3.8, "expected_speed": "fast"},
            "Mistral-7B": {"size_gb": 14.0, "expected_speed": "slow"}, 
            "Llama-3.2-3B": {"size_gb": 6.0, "expected_speed": "medium-fast"}
        }
        
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
            in pattern recognition, natural language processing, and decision-making tasks."""
        }
    
    def calculate_specialized_models_size(self, features):
        """Calculate total size for specialized models approach"""
        total_size = 0
        models_used = []
        
        feature_model_mapping = {
            "summarization": "summarization",
            "keywords": "keywords", 
            "grammar": "grammar",
            "ner": "ner",
            "tone": "tone",
            "rephrase": "rephrase"
        }
        
        for feature in features:
            if feature in feature_model_mapping:
                model_key = feature_model_mapping[feature]
                if model_key in self.specialized_models:
                    total_size += self.specialized_models[model_key]["size_gb"]
                    models_used.append(self.specialized_models[model_key]["model"])
        
        return total_size, models_used
    
    def test_endpoint_performance(self, server_url, endpoint, text, timeout=30):
        """Test endpoint performance with timeout protection"""
        start_time = time.time()
        
        try:
            response = requests.post(
                f"{server_url}/api/{endpoint}",
                json={"text": text},
                timeout=timeout
            )
            
            end_time = time.time()
            execution_time = end_time - start_time
            
            if response.status_code == 200:
                return {
                    "success": True,
                    "time_seconds": round(execution_time, 2),
                    "response": response.json(),
                    "timed_out": False
                }
            else:
                return {
                    "success": False,
                    "time_seconds": round(execution_time, 2),
                    "error": f"HTTP {response.status_code}",
                    "timed_out": False
                }
                
        except requests.exceptions.Timeout:
            return {
                "success": False,
                "time_seconds": timeout,
                "error": "Request timed out",
                "timed_out": True
            }
        except Exception as e:
            end_time = time.time()
            return {
                "success": False,
                "time_seconds": round(end_time - start_time, 2),
                "error": str(e),
                "timed_out": False
            }
    
    def run_packaging_analysis(self):
        """Run comprehensive packaging size vs performance analysis"""
        print("📦 TexDit Packaging Size vs Performance Analysis")
        print("=" * 60)
        
        # Define different feature sets for the app
        feature_scenarios = {
            "minimal": ["summarization", "keywords"],
            "standard": ["summarization", "keywords", "tone", "rephrase"], 
            "full": ["summarization", "keywords", "tone", "rephrase", "grammar", "ner"]
        }
        
        print("\n🎯 PACKAGING SIZE COMPARISON")
        print("-" * 40)
        
        for scenario_name, features in feature_scenarios.items():
            total_size, models = self.calculate_specialized_models_size(features)
            print(f"\n{scenario_name.upper()} APP ({len(features)} features):")
            print(f"  Specialized Models: {total_size:.1f}GB total")
            print(f"  Models needed: {len(models)}")
            
            print(f"  Single LLM alternatives:")
            for llm_name, llm_info in self.single_llm_options.items():
                size_diff = llm_info["size_gb"] - total_size
                if size_diff < 0:
                    print(f"    {llm_name}: {llm_info['size_gb']:.1f}GB (✅ {abs(size_diff):.1f}GB smaller)")
                else:
                    print(f"    {llm_name}: {llm_info['size_gb']:.1f}GB (❌ {size_diff:.1f}GB larger)")
        
        # Performance testing if servers are available
        print("\n⚡ PERFORMANCE TESTING")
        print("-" * 40)
        
        if self.test_server_health(self.current_server_url) and self.test_server_health(self.single_llm_server_url):
            self.run_performance_comparison()
        else:
            print("⚠️  Servers not available. Run test_manager.py first for performance data.")
            
        # Generate recommendations
        self.generate_recommendations()
    
    def test_server_health(self, url, timeout=5):
        """Check if server is healthy"""
        try:
            response = requests.get(f"{url}/health", timeout=timeout)
            return response.status_code == 200
        except:
            return False
    
    def run_performance_comparison(self):
        """Run quick performance test on available servers"""
        endpoints = ["summarise", "keywords", "tone", "rephrase"]
        
        print("\n🏃‍♂️ Quick Performance Test (Medium Text):")
        
        for endpoint in endpoints:
            print(f"\n  📝 Testing {endpoint}...")
            
            # Test current (specialized) approach
            current_result = self.test_endpoint_performance(
                self.current_server_url, endpoint, self.test_texts["medium"], timeout=15
            )
            
            # Test single LLM approach  
            single_result = self.test_endpoint_performance(
                self.single_llm_server_url, endpoint, self.test_texts["medium"], timeout=15
            )
            
            # Display results
            if current_result["success"]:
                print(f"    Specialized: ✅ {current_result['time_seconds']}s")
            else:
                print(f"    Specialized: ❌ {current_result['error']}")
            
            if single_result["success"]:
                print(f"    Single LLM:  ✅ {single_result['time_seconds']}s")
                
                # Check if single LLM is acceptable (< 10s threshold)
                if single_result['time_seconds'] < 5:
                    print(f"    📊 Single LLM acceptable speed ✅")
                elif single_result['time_seconds'] < 10:
                    print(f"    📊 Single LLM borderline speed ⚠️")
                else:
                    print(f"    📊 Single LLM too slow ❌")
            else:
                print(f"    Single LLM:  ❌ {single_result['error']}")
                if single_result["timed_out"]:
                    print(f"    📊 Single LLM too slow (>15s) ❌")
    
    def generate_recommendations(self):
        """Generate packaging and architecture recommendations"""
        print("\n" + "=" * 60)
        print("💡 PACKAGING RECOMMENDATIONS")
        print("=" * 60)
        
        print("\n🎯 SIZE ANALYSIS:")
        print("  • Minimal app (2 features): 1.6GB specialized vs 3.0-3.8GB single LLM")
        print("  • Standard app (4 features): 4.4GB specialized vs 3.0-3.8GB single LLM") 
        print("  • Full app (6 features): 6.0GB specialized vs 3.0-3.8GB single LLM")
        
        print("\n✅ SWEET SPOT FOUND:")
        print("  • Standard+ apps: Single LLM wins on size (3.8GB vs 4.4GB+)")
        print("  • Flan-T5-Large (3GB) or Phi-3-Mini (3.8GB) are optimal")
        print("  • Avoid Mistral-7B (14GB) - too large for offline deployment")
        
        print("\n⚡ PERFORMANCE THRESHOLDS:")
        print("  • Target: < 5 seconds per query (excellent UX)")
        print("  • Acceptable: 5-10 seconds (decent UX)")  
        print("  • Unacceptable: > 10 seconds (poor UX)")
        
        print("\n🏆 FINAL RECOMMENDATION:")
        print("  1. For 3+ features: Use Single LLM (Flan-T5-Large or Phi-3-Mini)")
        print("  2. Package size: 3.0-3.8GB vs 4.4-6.0GB specialized")
        print("  3. User experience: Test actual performance on target hardware")
        print("  4. Quality trade-off: Single LLM may have slightly lower quality")
        
        print("\n🔧 IMPLEMENTATION STRATEGY:")
        print("  • Start with Flan-T5-Large (3GB, proven instruction following)")
        print("  • Add performance optimizations (caching, beam=1)")
        print("  • Test on mid-range hardware (8GB RAM, integrated graphics)")
        print("  • Have fallback to specialized models if speed unacceptable")

def main():
    analyzer = PackagingSizeAnalysis()
    analyzer.run_packaging_analysis()

if __name__ == "__main__":
    main()
