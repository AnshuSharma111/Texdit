"""
TexDit Architecture Performance Test
Compare: Multiple Small Models vs Single Large LLM

This script tests both architectures to measure:
1. Model loading time
2. Memory usage
3. Inference speed per task
4. Overall responsiveness
5. Quality of outputs
"""

import time
import psutil
import os
import json
from datetime import datetime
import torch
from transformers import AutoTokenizer, AutoModelForSeq2SeqLM, AutoModelForCausalLM, pipeline

# Test texts for benchmarking
TEST_TEXTS = {
    "short": "The quick brown fox jumps over the lazy dog. This is a simple test sentence for evaluation.",
    "medium": """
    Artificial intelligence has revolutionized many aspects of modern life, from healthcare to transportation. 
    Machine learning algorithms can now diagnose diseases, predict weather patterns, and even create art. 
    However, with these advancements come new challenges and ethical considerations. Privacy concerns, job displacement, 
    and the potential for bias in AI systems are all important issues that society must address as we continue 
    to integrate these technologies into our daily lives.
    """,
    "long": """
    The field of artificial intelligence has undergone remarkable transformations over the past decade, 
    fundamentally altering how we approach complex problems across numerous domains. From healthcare systems 
    that can predict patient outcomes with unprecedented accuracy to autonomous vehicles navigating busy city streets, 
    AI technologies have moved from the realm of science fiction into practical, everyday applications. 
    
    Machine learning algorithms, particularly deep learning models, have demonstrated exceptional capabilities 
    in pattern recognition, natural language processing, and decision-making tasks. These systems can now 
    analyze vast amounts of data in seconds, identifying trends and insights that would take human analysts 
    weeks or months to discover. In medical imaging, AI can detect early-stage cancers with greater precision 
    than experienced radiologists. In finance, algorithmic trading systems process millions of transactions 
    per second, making split-second decisions based on market conditions.
    
    However, this rapid advancement brings significant challenges that society must carefully navigate. 
    Privacy concerns have become paramount as AI systems require enormous amounts of personal data to function 
    effectively. The potential for algorithmic bias raises questions about fairness and equality in automated 
    decision-making processes. Job displacement due to automation affects entire industries, requiring 
    comprehensive retraining programs and social safety nets.
    """
}

class ArchitectureTest:
    def __init__(self):
        self.results = {
            "timestamp": datetime.now().isoformat(),
            "system_info": self.get_system_info(),
            "current_architecture": {},
            "single_llm_architecture": {}
        }
        
    def get_system_info(self):
        """Get system specifications"""
        return {
            "cpu_count": psutil.cpu_count(),
            "memory_total_gb": round(psutil.virtual_memory().total / (1024**3), 2),
            "memory_available_gb": round(psutil.virtual_memory().available / (1024**3), 2),
            "python_version": os.sys.version,
            "torch_version": torch.__version__,
            "cuda_available": torch.cuda.is_available()
        }
    
    def measure_memory_usage(self, func, *args, **kwargs):
        """Measure memory usage of a function"""
        process = psutil.Process()
        memory_before = process.memory_info().rss / (1024**2)  # MB
        
        start_time = time.time()
        result = func(*args, **kwargs)
        end_time = time.time()
        
        memory_after = process.memory_info().rss / (1024**2)  # MB
        
        return {
            "result": result,
            "execution_time": round(end_time - start_time, 3),
            "memory_used_mb": round(memory_after - memory_before, 2),
            "memory_peak_mb": round(memory_after, 2)
        }
    
    def test_current_architecture(self):
        """Test the current DistilBART + rule-based approach"""
        print("🔄 Testing Current Architecture (DistilBART + Rules)...")
        
        # Test model loading
        print("  📦 Loading DistilBART model...")
        def load_current_model():
            local_model_path = os.path.join("..", "models", "distilbart-cnn-12-6")
            tokenizer = AutoTokenizer.from_pretrained(local_model_path, local_files_only=True)
            model = AutoModelForSeq2SeqLM.from_pretrained(local_model_path, local_files_only=True)
            return tokenizer, model
        
        loading_metrics = self.measure_memory_usage(load_current_model)
        tokenizer, model = loading_metrics["result"]
        
        self.results["current_architecture"]["loading"] = {
            "time_seconds": loading_metrics["execution_time"],
            "memory_mb": loading_metrics["memory_used_mb"],
            "model_size_description": "DistilBART-CNN-12-6 (~1.14GB)"
        }
        
        # Test each functionality
        tasks = ["summarize", "keywords", "tone", "rephrase"]
        for task in tasks:
            print(f"  🧪 Testing {task}...")
            task_results = {}
            
            for text_size, text in TEST_TEXTS.items():
                if task == "summarize":
                    def run_task():
                        return self.run_distilbart_summarize(tokenizer, model, text)
                elif task == "rephrase":
                    def run_task():
                        return self.run_distilbart_rephrase(tokenizer, model, text)
                elif task == "keywords":
                    def run_task():
                        return self.run_rule_based_keywords(text)
                elif task == "tone":
                    def run_task():
                        return self.run_rule_based_tone(text)
                
                metrics = self.measure_memory_usage(run_task)
                task_results[text_size] = {
                    "time_seconds": metrics["execution_time"],
                    "memory_mb": metrics["memory_used_mb"],
                    "output_preview": str(metrics["result"])[:100] + "..." if len(str(metrics["result"])) > 100 else str(metrics["result"])
                }
            
            self.results["current_architecture"][task] = task_results
        
        print("✅ Current architecture testing complete!")
    
    def test_single_llm_architecture(self):
        """Test single LLM approach with different model options"""
        print("\n🔄 Testing Single LLM Architecture...")
        
        # Test different LLM options for packaging size comparison
        llm_options = [
            {
                "name": "Flan-T5-Small",
                "model_id": "google/flan-t5-small", 
                "size_description": "~310MB",
                "type": "seq2seq"
            },
            {
                "name": "Flan-T5-Base",
                "model_id": "google/flan-t5-base",
                "size_description": "~990MB",
                "type": "seq2seq"
            },
            {
                "name": "Flan-T5-Large", 
                "model_id": "google/flan-t5-large",
                "size_description": "~3GB",
                "type": "seq2seq"
            },
            # Modern efficient models for offline deployment
            {
                "name": "Phi-3-Mini-Instruct",
                "model_id": "microsoft/Phi-3-mini-4k-instruct",
                "size_description": "~3.8GB",
                "type": "causal"
            },
            # Note: Mistral 7B would be ~14GB, likely too slow for this test
            # Focusing on models that could realistically replace 6-7GB of specialized models
        ]
        
        for llm_config in llm_options:
            print(f"  📦 Testing {llm_config['name']}...")
            
            # Test model loading
            def load_llm():
                if llm_config["type"] == "seq2seq":
                    tokenizer = AutoTokenizer.from_pretrained(llm_config["model_id"])
                    model = AutoModelForSeq2SeqLM.from_pretrained(llm_config["model_id"])
                else:
                    tokenizer = AutoTokenizer.from_pretrained(llm_config["model_id"])
                    model = AutoModelForCausalLM.from_pretrained(llm_config["model_id"])
                return tokenizer, model
            
            try:
                loading_metrics = self.measure_memory_usage(load_llm)
                tokenizer, model = loading_metrics["result"]
                
                llm_results = {
                    "loading": {
                        "time_seconds": loading_metrics["execution_time"],
                        "memory_mb": loading_metrics["memory_used_mb"],
                        "model_size_description": llm_config["size_description"]
                    }
                }
                
                # Test all tasks with the single LLM
                tasks = ["summarize", "keywords", "tone", "rephrase"]
                for task in tasks:
                    print(f"    🧪 Testing {task} with {llm_config['name']}...")
                    task_results = {}
                    
                    for text_size, text in TEST_TEXTS.items():
                        def run_task():
                            return self.run_llm_task(tokenizer, model, task, text, llm_config["type"])
                        
                        metrics = self.measure_memory_usage(run_task)
                        task_results[text_size] = {
                            "time_seconds": metrics["execution_time"],
                            "memory_mb": metrics["memory_used_mb"],
                            "output_preview": str(metrics["result"])[:100] + "..." if len(str(metrics["result"])) > 100 else str(metrics["result"])
                        }
                    
                    llm_results[task] = task_results
                
                self.results["single_llm_architecture"][llm_config["name"]] = llm_results
                print(f"✅ {llm_config['name']} testing complete!")
                
            except Exception as e:
                print(f"❌ Failed to test {llm_config['name']}: {e}")
                self.results["single_llm_architecture"][llm_config["name"]] = {"error": str(e)}
    
    def run_distilbart_summarize(self, tokenizer, model, text):
        """Run DistilBART summarization (current approach)"""
        inputs = tokenizer(text, return_tensors="pt", max_length=1024, truncation=True, padding=True)
        summary_ids = model.generate(
            inputs["input_ids"],
            attention_mask=inputs["attention_mask"],
            max_length=150,
            min_length=30,
            length_penalty=1.5,
            num_beams=2,
            early_stopping=True,
            no_repeat_ngram_size=3,
            do_sample=False
        )
        return tokenizer.decode(summary_ids[0], skip_special_tokens=True)
    
    def run_distilbart_rephrase(self, tokenizer, model, text):
        """Run DistilBART rephrasing (current approach)"""
        inputs = tokenizer(text, return_tensors="pt", max_length=512, truncation=True, padding=True)
        outputs = model.generate(
            inputs["input_ids"],
            attention_mask=inputs.get('attention_mask', None),
            max_length=len(text.split()) + 50,
            min_length=max(5, len(text.split()) - 10),
            length_penalty=2.0,
            num_beams=4,
            early_stopping=True,
            do_sample=True,
            temperature=0.8
        )
        return tokenizer.decode(outputs[0], skip_special_tokens=True)
    
    def run_rule_based_keywords(self, text):
        """Run rule-based keyword extraction (current approach)"""
        words = text.lower().split()
        stop_words = {'the', 'a', 'an', 'and', 'or', 'but', 'in', 'on', 'at', 'to', 'for', 'of', 'with', 'by'}
        keywords = [word.strip('.,!?;:"()[]') for word in words if word not in stop_words and len(word) > 3]
        keyword_freq = {}
        for keyword in keywords:
            keyword_freq[keyword] = keyword_freq.get(keyword, 0) + 1
        top_keywords = sorted(keyword_freq.items(), key=lambda x: x[1], reverse=True)[:10]
        return [kw[0] for kw in top_keywords]
    
    def run_rule_based_tone(self, text):
        """Run rule-based tone analysis (current approach)"""
        text_lower = text.lower()
        positive_words = ['good', 'great', 'excellent', 'amazing', 'wonderful']
        negative_words = ['bad', 'terrible', 'awful', 'hate', 'angry']
        formal_words = ['therefore', 'furthermore', 'however', 'consequently']
        
        positive_count = sum(1 for word in positive_words if word in text_lower)
        negative_count = sum(1 for word in negative_words if word in text_lower)
        formal_count = sum(1 for word in formal_words if word in text_lower)
        
        tone = "positive" if positive_count > negative_count else ("negative" if negative_count > positive_count else "neutral")
        formality = "formal" if formal_count > 2 else "informal"
        
        return {"tone": tone, "formality": formality}
    
    def run_llm_task(self, tokenizer, model, task, text, model_type):
        """Run any task using a single LLM with prompt engineering"""
        prompts = {
            "summarize": f"Summarize the following text concisely:\n\n{text}\n\nSummary:",
            "keywords": f"Extract the most important keywords from this text:\n\n{text}\n\nKeywords:",
            "tone": f"Analyze the tone and formality of this text:\n\n{text}\n\nTone analysis:",
            "rephrase": f"Rephrase the following text while keeping the same meaning:\n\n{text}\n\nRephrased:"
        }
        
        prompt = prompts[task]
        
        if model_type == "seq2seq":
            inputs = tokenizer(prompt, return_tensors="pt", max_length=1024, truncation=True)
            outputs = model.generate(
                inputs["input_ids"],
                max_length=200,
                min_length=10,
                num_beams=2,
                early_stopping=True,
                pad_token_id=tokenizer.eos_token_id
            )
            return tokenizer.decode(outputs[0], skip_special_tokens=True)
        else:
            # For causal LM models
            inputs = tokenizer(prompt, return_tensors="pt", max_length=1024, truncation=True)
            outputs = model.generate(
                inputs["input_ids"],
                max_length=inputs["input_ids"].shape[1] + 100,
                num_beams=2,
                early_stopping=True,
                pad_token_id=tokenizer.eos_token_id
            )
            response = tokenizer.decode(outputs[0], skip_special_tokens=True)
            # Extract just the generated part (after the prompt)
            return response[len(prompt):].strip()
    
    def save_results(self):
        """Save test results to JSON file"""
        filename = f"architecture_comparison_{datetime.now().strftime('%Y%m%d_%H%M%S')}.json"
        with open(filename, 'w') as f:
            json.dump(self.results, f, indent=2)
        print(f"\n📊 Results saved to: {filename}")
        return filename
    
    def print_summary(self):
        """Print a summary of key findings"""
        print("\n" + "="*60)
        print("🏆 ARCHITECTURE COMPARISON SUMMARY")
        print("="*60)
        
        # Current architecture summary
        current = self.results["current_architecture"]
        if "loading" in current:
            print(f"\n📊 CURRENT ARCHITECTURE (DistilBART + Rules)")
            print(f"  Loading time: {current['loading']['time_seconds']}s")
            print(f"  Memory usage: {current['loading']['memory_mb']}MB")
            
            # Average task times
            avg_times = {}
            for task in ["summarize", "rephrase", "keywords", "tone"]:
                if task in current:
                    times = [current[task][size]["time_seconds"] for size in TEST_TEXTS.keys()]
                    avg_times[task] = round(sum(times) / len(times), 3)
            
            print(f"  Average task times:")
            for task, time_val in avg_times.items():
                print(f"    {task}: {time_val}s")
        
        # Single LLM summary
        print(f"\n📊 SINGLE LLM ARCHITECTURES")
        for llm_name, llm_data in self.results["single_llm_architecture"].items():
            if "error" not in llm_data and "loading" in llm_data:
                print(f"\n  {llm_name}:")
                print(f"    Loading time: {llm_data['loading']['time_seconds']}s")
                print(f"    Memory usage: {llm_data['loading']['memory_mb']}MB")
                
                # Average task times
                avg_times = {}
                for task in ["summarize", "rephrase", "keywords", "tone"]:
                    if task in llm_data:
                        times = [llm_data[task][size]["time_seconds"] for size in TEST_TEXTS.keys()]
                        avg_times[task] = round(sum(times) / len(times), 3)
                
                if avg_times:
                    print(f"    Average task times:")
                    for task, time_val in avg_times.items():
                        print(f"      {task}: {time_val}s")
            elif "error" in llm_data:
                print(f"\n  {llm_name}: ❌ {llm_data['error']}")

def main():
    print("🚀 TexDit Architecture Performance Test")
    print("="*50)
    
    tester = ArchitectureTest()
    
    # Test current architecture
    tester.test_current_architecture()
    
    # Test single LLM architecture
    tester.test_single_llm_architecture()
    
    # Save and summarize results
    filename = tester.save_results()
    tester.print_summary()
    
    print(f"\n📋 RECOMMENDATIONS:")
    print(f"1. Check detailed results in: {filename}")
    print(f"2. Consider user experience threshold: < 3 seconds per query")
    print(f"3. Evaluate quality vs speed trade-offs")
    print(f"4. Consider hybrid approach for different tasks")

if __name__ == "__main__":
    main()
