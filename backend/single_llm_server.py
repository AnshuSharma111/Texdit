"""
Single LLM Test Server
Test implementation of TexDit with a single large language model
"""

from flask import Flask, jsonify, request
from flask_cors import CORS
from transformers import AutoTokenizer, AutoModelForSeq2SeqLM
import time
import logging

# Set up logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

app = Flask(__name__)
CORS(app)

# Global variables for model
tokenizer = None
model = None
model_name = "google/flan-t5-large"  # Test larger model for realistic comparison

def load_single_llm():
    """Load the single LLM for all tasks"""
    global tokenizer, model
    
    logger.info(f"Loading single LLM: {model_name}")
    start_time = time.time()
    
    try:
        tokenizer = AutoTokenizer.from_pretrained(model_name)
        model = AutoModelForSeq2SeqLM.from_pretrained(model_name)
        
        load_time = time.time() - start_time
        logger.info(f"✅ Single LLM loaded successfully in {load_time:.2f} seconds")
        return True
        
    except Exception as e:
        logger.error(f"❌ Failed to load single LLM: {e}")
        return False

def generate_with_llm(prompt, max_length=200, min_length=10):
    """Generate response using the single LLM"""
    if not model or not tokenizer:
        raise Exception("Model not loaded")
    
    inputs = tokenizer(prompt, return_tensors="pt", max_length=1024, truncation=True, padding=True)
    
    outputs = model.generate(
        inputs["input_ids"],
        attention_mask=inputs.get("attention_mask"),
        max_length=max_length,
        min_length=min_length,
        num_beams=2,
        early_stopping=True,
        no_repeat_ngram_size=2,
        pad_token_id=tokenizer.eos_token_id
    )
    
    response = tokenizer.decode(outputs[0], skip_special_tokens=True)
    return response.strip()

@app.route('/health')
def health():
    """Health check endpoint"""
    return jsonify({
        "status": "ok",
        "message": "Single LLM server is healthy",
        "model_loaded": model is not None,
        "model_name": model_name
    })

@app.route('/api/summarise', methods=['POST'])
def summarise():
    """Summarize using single LLM"""
    try:
        data = request.get_json()
        
        if not data or 'text' not in data:
            return jsonify({"error": "Missing required field: 'text'"}), 400
        
        text = data['text'].strip()
        ratio = data.get('ratio', 0.25)
        
        if not text:
            return jsonify({"error": "Text cannot be empty"}), 400
        
        # Calculate target length
        original_word_count = len(text.split())
        target_words = max(10, int(original_word_count * ratio))
        
        # Create summarization prompt
        prompt = f"""Summarize the following text in approximately {target_words} words:

Text: {text}

Summary:"""
        
        start_time = time.time()
        
        # Generate summary
        summary = generate_with_llm(prompt, max_length=target_words + 50, min_length=max(5, target_words - 20))
        
        # Clean up the response (remove prompt echo if present)
        if "Summary:" in summary:
            summary = summary.split("Summary:")[-1].strip()
        
        end_time = time.time()
        
        return jsonify({
            "summary": summary,
            "original_length": original_word_count,
            "summary_length": len(summary.split()),
            "compression_ratio": round(len(summary.split()) / original_word_count, 3),
            "performance": {
                "total_time": round(end_time - start_time, 2)
            },
            "model_used": model_name
        })
        
    except Exception as e:
        logger.error(f"Error in summarise: {str(e)}")
        return jsonify({"error": str(e)}), 500

@app.route('/api/keywords', methods=['POST'])
def keywords():
    """Extract keywords using single LLM"""
    try:
        data = request.get_json()
        
        if not data or 'text' not in data:
            return jsonify({"error": "Missing required field: 'text'"}), 400
        
        text = data['text'].strip()
        
        if not text:
            return jsonify({"error": "Text cannot be empty"}), 400
        
        prompt = f"""Extract the 10 most important keywords or key phrases from the following text. List them separated by commas:

Text: {text}

Keywords:"""
        
        start_time = time.time()
        
        keywords_response = generate_with_llm(prompt, max_length=100, min_length=5)
        
        # Clean up and parse keywords
        if "Keywords:" in keywords_response:
            keywords_response = keywords_response.split("Keywords:")[-1].strip()
        
        # Split by commas and clean up
        keywords = [kw.strip() for kw in keywords_response.split(',') if kw.strip()]
        
        end_time = time.time()
        
        return jsonify({
            "keywords": keywords[:10],  # Limit to 10
            "performance": {
                "total_time": round(end_time - start_time, 2)
            },
            "model_used": model_name
        })
        
    except Exception as e:
        logger.error(f"Error in keywords: {str(e)}")
        return jsonify({"error": str(e)}), 500

@app.route('/api/tone', methods=['POST'])
def tone():
    """Analyze tone using single LLM"""
    try:
        data = request.get_json()
        
        if not data or 'text' not in data:
            return jsonify({"error": "Missing required field: 'text'"}), 400
        
        text = data['text'].strip()
        
        if not text:
            return jsonify({"error": "Text cannot be empty"}), 400
        
        prompt = f"""Analyze the tone and formality of the following text. Respond with:
Tone: [positive/negative/neutral]
Formality: [formal/informal]

Text: {text}

Analysis:"""
        
        start_time = time.time()
        
        analysis = generate_with_llm(prompt, max_length=50, min_length=5)
        
        # Parse the response
        tone = "neutral"
        formality = "informal"
        
        analysis_lower = analysis.lower()
        if "positive" in analysis_lower:
            tone = "positive"
        elif "negative" in analysis_lower:
            tone = "negative"
        
        if "formal" in analysis_lower and "informal" not in analysis_lower:
            formality = "formal"
        
        end_time = time.time()
        
        return jsonify({
            "tone": tone,
            "formality": formality,
            "analysis": {
                "raw_response": analysis,
                "confidence": "medium"  # Could be enhanced with more sophisticated parsing
            },
            "performance": {
                "total_time": round(end_time - start_time, 2)
            },
            "model_used": model_name
        })
        
    except Exception as e:
        logger.error(f"Error in tone: {str(e)}")
        return jsonify({"error": str(e)}), 500

@app.route('/api/rephrase', methods=['POST'])
def rephrase():
    """Rephrase text using single LLM"""
    try:
        data = request.get_json()
        
        if not data or 'text' not in data:
            return jsonify({"error": "Missing required field: 'text'"}), 400
        
        text = data['text'].strip()
        
        if not text:
            return jsonify({"error": "Text cannot be empty"}), 400
        
        prompt = f"""Rewrite the following text using different words while keeping the same meaning:

Original: {text}

Rewritten:"""
        
        start_time = time.time()
        
        rephrased = generate_with_llm(prompt, max_length=len(text.split()) + 100, min_length=max(5, len(text.split()) - 10))
        
        # Clean up the response
        if "Rewritten:" in rephrased:
            rephrased = rephrased.split("Rewritten:")[-1].strip()
        
        end_time = time.time()
        
        return jsonify({
            "original": text,
            "rephrased": rephrased,
            "performance": {
                "total_time": round(end_time - start_time, 2)
            },
            "model_used": model_name
        })
        
    except Exception as e:
        logger.error(f"Error in rephrase: {str(e)}")
        return jsonify({"error": str(e)}), 500

@app.route('/api/switch_model', methods=['POST'])
def switch_model():
    """Switch to a different LLM for testing"""
    global model_name, tokenizer, model
    
    try:
        data = request.get_json()
        
        if not data or 'model_name' not in data:
            return jsonify({"error": "Missing required field: 'model_name'"}), 400
        
        new_model_name = data['model_name']
        
        # List of supported models for testing
        supported_models = [
            "google/flan-t5-base",
            "google/flan-t5-large",
            "google/flan-t5-small"
        ]
        
        if new_model_name not in supported_models:
            return jsonify({
                "error": f"Model not supported. Supported models: {supported_models}"
            }), 400
        
        # Clear current model
        model = None
        tokenizer = None
        model_name = new_model_name
        
        # Load new model
        success = load_single_llm()
        
        if success:
            return jsonify({
                "message": f"Successfully switched to {model_name}",
                "model_name": model_name
            })
        else:
            return jsonify({"error": "Failed to load new model"}), 500
            
    except Exception as e:
        logger.error(f"Error switching model: {str(e)}")
        return jsonify({"error": str(e)}), 500

if __name__ == '__main__':
    # Load the model at startup
    if load_single_llm():
        logger.info("🚀 Starting Single LLM Test Server...")
        app.run(debug=False, host='0.0.0.0', port=5001, use_reloader=False)
    else:
        logger.error("❌ Failed to start server - model loading failed")
