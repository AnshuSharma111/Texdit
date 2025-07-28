from rapidfuzz import fuzz, process # Fuzzy search library
import time # For speed benchmarking

from flask import Flask, jsonify, request # Server framework
from flask_cors import CORS # Enable CORS for Qt integration

from transformers import AutoTokenizer, AutoModelForSeq2SeqLM # Model for summarization
import logging

# Set up logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

app = Flask(__name__)
CORS(app)  # Enable CORS for all routes

# Initialize the tokenizer and model for summarization
# Using DistilBART-CNN for better summarization quality and length control

import os
local_model_path = os.path.join(os.path.dirname(__file__), "..", "models", "distilbart-cnn-12-6")
logger.info(f"Loading DistilBART model from local directory: {local_model_path}")

# Load from local model directory (packaged with the app)
try:
    tokenizer = AutoTokenizer.from_pretrained(local_model_path, local_files_only=True)
    model = AutoModelForSeq2SeqLM.from_pretrained(local_model_path, local_files_only=True)
    logger.info("Successfully loaded DistilBART model from local directory")
except Exception as e:
    logger.error(f"Failed to load local model: {e}")
    # Fallback to Hugging Face if local model fails
    model_name = "sshleifer/distilbart-cnn-12-6"
    logger.info("Falling back to downloading from Hugging Face...")
    tokenizer = AutoTokenizer.from_pretrained(model_name)
    model = AutoModelForSeq2SeqLM.from_pretrained(model_name)
    logger.info("Successfully loaded DistilBART model from Hugging Face")

def fuzzy_search(query, choices, limit=10):
    """Perform fuzzy search using rapidfuzz"""
    results = process.extract(query, choices, scorer=fuzz.ratio, limit=limit)
    return results

@app.route('/')
def home():
    """Basic health check endpoint"""
    return jsonify({
        "message": "API is running",
        "version": "1.0.0",
        "model_loaded": model is not None,
        "endpoints": ["/api/search", "/api/summarise"]
    })

@app.route('/health')
def health():
    """Health check endpoint for monitoring"""
    return jsonify({
        "status": "ok",
        "message": "Server is healthy",
        "model_loaded": model is not None
    })

@app.route('/api/search', methods=['POST'])
def search():
    """Fuzzy search endpoint"""
    try:
        data = request.get_json()

        # Validate input     
        if not data or 'query' not in data or 'choices' not in data:
            return jsonify({
                "error": "Missing required fields: 'query' and 'choices'"
            }), 400
        
        query = data['query']
        choices = data['choices']
        limit = data.get('limit', 10)
        
        # Perform fuzzy search
        results = fuzzy_search(query, choices, limit)
        
        # Return Results
        return jsonify({
            "results":[x[0] for x in results]
        })
    
    except Exception as e:
        logger.error(f"Error occurred in /api/search: {str(e)}")
        return jsonify({"error": str(e)}), 500

@app.route('/api/summarise', methods=["POST"])
def summarise():
    """Summarise endpoint"""
    try:
        data = request.get_json()

        # Validate input
        if not data or 'text' not in data:
            return jsonify({
                "error": "Missing required field: 'text'"
            }), 400
        
        text = data['text'].strip()
        ratio = data.get('ratio', 0.25) # Default length of summarise is 25% of original text length
        min_ratio = data.get('min_ratio', ratio * 0.8)  # Default to 80% of target ratio
        max_ratio = data.get('max_ratio', ratio * 1.2)  # Default to 120% of target ratio
        
        # Validate ratio
        if not isinstance(ratio, (int, float)) or ratio <= 0 or ratio > 1:
            return jsonify({
                "error": "Ratio must be a number between 0 and 1"
            }), 400
        
        # Validate min/max ratios
        if not isinstance(min_ratio, (int, float)) or min_ratio <= 0 or min_ratio > 1:
            min_ratio = max(0.05, ratio * 0.8)  # Fallback to 80% of target, minimum 5%
            
        if not isinstance(max_ratio, (int, float)) or max_ratio <= 0 or max_ratio > 1:
            max_ratio = min(1.0, ratio * 1.2)   # Fallback to 120% of target, maximum 100%
        
        # Validate text length
        if not text:
            return jsonify({
                "error": "Text cannot be empty"
            }), 400
        
        if len(text.split()) < 10:
            return jsonify({
                "error": "Text must be at least 10 words long for meaningful summarization"
            }), 400
        
        if len(text) > 10000:  # Limit to prevent memory issues
            return jsonify({
                "error": "Text too long. Maximum 10,000 characters allowed."
            }), 400

        # Calculate target summary length based on ratios
        original_word_count = len(text.split())
        target_length = max(10, int(original_word_count * ratio))  # Ensure minimum of 10 words
        min_length = max(5, int(original_word_count * min_ratio))  # Minimum based on min_ratio
        
        # Remove arbitrary 150-word cap that breaks large summaries
        # Allow up to 80% of original length for very high ratio requests
        absolute_max = max(150, int(original_word_count * 0.8))  # Dynamic cap based on input size
        max_length = min(int(original_word_count * max_ratio) + 10, absolute_max)
        
        # Ensure logical constraints
        min_length = min(min_length, target_length - 5)  # Min should be less than target
        max_length = max(max_length, target_length + 5)  # Max should be more than target
        
        logger.info(f"Summary parameters: original={original_word_count} words, "
                   f"target={target_length} words ({ratio:.1%}), "
                   f"range={min_length}-{max_length} words "
                   f"({min_ratio:.1%}-{max_ratio:.1%})")

        # Start timing for performance monitoring
        start_time = time.time()

        # BART doesn't need task prefix like T5 - just use the text directly
        inputs = tokenizer(text, return_tensors="pt", max_length=1024, truncation=True, padding=True)
        
        tokenization_time = time.time()
        
        # Generate summary with BART-optimized parameters (faster settings for better responsiveness)
        summary_ids = model.generate(
            inputs["input_ids"],
            attention_mask=inputs["attention_mask"],  # Important for BART
            max_length=max_length, 
            min_length=min_length, 
            length_penalty=1.5,       # Slightly reduced for faster generation
            num_beams=2,              # Reduced from 4 to 2 for faster inference
            early_stopping=True,      # Stop when EOS is reached
            no_repeat_ngram_size=3,   # Prevent repetition
            do_sample=False,          # Deterministic generation
            forced_bos_token_id=tokenizer.bos_token_id  # Ensure proper start token
        )

        generation_time = time.time()
        summary = tokenizer.decode(summary_ids[0], skip_special_tokens=True)
        
        # Clean up any remaining artifacts
        summary = summary.strip()
        
        # Additional cleanup for malformed output
        if summary.endswith(('..', '...', '....', 'and..', 'a.', 'the.', "it '", "'the.")):
            # Find the last complete sentence
            sentences = summary.split('.')
            if len(sentences) > 1:
                # Keep all complete sentences except the last incomplete one
                summary = '.'.join(sentences[:-1]) + '.'
        
        # Ensure summary is not empty after cleanup
        if not summary.strip():
            summary = "Summary could not be generated properly. Please try again with different text."
        
        end_time = time.time()
        
        # Calculate timing metrics
        total_time = end_time - start_time
        tokenization_duration = tokenization_time - start_time
        generation_duration = generation_time - tokenization_time
        decoding_duration = end_time - generation_time
        
        logger.info(f"Performance metrics: Total={total_time:.2f}s, "
                   f"Tokenization={tokenization_duration:.2f}s, "
                   f"Generation={generation_duration:.2f}s, "
                   f"Decoding={decoding_duration:.2f}s")
        
        return jsonify({
            "summary": summary,
            "original_length": len(text.split()),
            "summary_length": len(summary.split()),
            "compression_ratio": round(len(summary.split()) / len(text.split()), 3),
            "performance": {
                "total_time": round(total_time, 2),
                "tokenization_time": round(tokenization_duration, 2),
                "generation_time": round(generation_duration, 2),
                "decoding_time": round(decoding_duration, 2)
            }
        })
    except Exception as e:
        logger.error(f"Error occurred in /api/summarise: {str(e)}")
        return jsonify({"error": str(e)}), 500

@app.route('/api/keywords', methods=["POST"])
def keywords():
    """Extract keywords endpoint"""
    try:
        data = request.get_json()
        
        if not data or 'text' not in data:
            return jsonify({
                "error": "Missing required field: 'text'"
            }), 400
        
        text = data['text'].strip()
        
        if not text:
            return jsonify({
                "error": "Text cannot be empty"
            }), 400
        
        # Simple keyword extraction (you can enhance this with NLP libraries)
        words = text.lower().split()
        # Remove common stop words
        stop_words = {'the', 'a', 'an', 'and', 'or', 'but', 'in', 'on', 'at', 'to', 'for', 'of', 'with', 'by', 'is', 'are', 'was', 'were', 'be', 'been', 'being', 'have', 'has', 'had', 'do', 'does', 'did', 'will', 'would', 'could', 'should', 'may', 'might', 'must', 'can', 'this', 'that', 'these', 'those'}
        keywords = [word.strip('.,!?;:"()[]') for word in words if word not in stop_words and len(word) > 3]
        
        # Get unique keywords and their frequency
        keyword_freq = {}
        for keyword in keywords:
            keyword_freq[keyword] = keyword_freq.get(keyword, 0) + 1
        
        # Sort by frequency and get top keywords
        top_keywords = sorted(keyword_freq.items(), key=lambda x: x[1], reverse=True)[:10]
        
        return jsonify({
            "keywords": [kw[0] for kw in top_keywords],
            "keyword_frequencies": dict(top_keywords)
        })
    except Exception as e:
        logger.error(f"Error occurred in /api/keywords: {str(e)}")
        return jsonify({"error": str(e)}), 500

@app.route('/api/tone', methods=["POST"])
def tone():
    """Analyze tone endpoint"""
    try:
        data = request.get_json()
        
        if not data or 'text' not in data:
            return jsonify({
                "error": "Missing required field: 'text'"
            }), 400
        
        text = data['text'].strip()
        
        if not text:
            return jsonify({
                "error": "Text cannot be empty"
            }), 400
        
        # Simple tone analysis (you can enhance this with sentiment analysis libraries)
        text_lower = text.lower()
        
        # Count different tone indicators
        positive_words = ['good', 'great', 'excellent', 'amazing', 'wonderful', 'fantastic', 'love', 'like', 'happy', 'pleased']
        negative_words = ['bad', 'terrible', 'awful', 'hate', 'dislike', 'angry', 'sad', 'disappointed', 'frustrated']
        formal_words = ['therefore', 'furthermore', 'however', 'consequently', 'nevertheless', 'moreover']
        
        positive_count = sum(1 for word in positive_words if word in text_lower)
        negative_count = sum(1 for word in negative_words if word in text_lower)
        formal_count = sum(1 for word in formal_words if word in text_lower)
        
        # Determine tone
        if positive_count > negative_count:
            tone = "positive"
        elif negative_count > positive_count:
            tone = "negative"
        else:
            tone = "neutral"
        
        if formal_count > 2:
            formality = "formal"
        else:
            formality = "informal"
        
        return jsonify({
            "tone": tone,
            "formality": formality,
            "analysis": {
                "positive_indicators": positive_count,
                "negative_indicators": negative_count,
                "formal_indicators": formal_count
            }
        })
    except Exception as e:
        logger.error(f"Error occurred in /api/tone: {str(e)}")
        return jsonify({"error": str(e)}), 500

@app.route('/api/rephrase', methods=["POST"])
def rephrase():
    """Rephrase text endpoint"""
    try:
        data = request.get_json()
        
        if not data or 'text' not in data:
            return jsonify({
                "error": "Missing required field: 'text'"
            }), 400
        
        text = data['text'].strip()
        
        if not text:
            return jsonify({
                "error": "Text cannot be empty"
            }), 400
        
        # Use DistilBART model for paraphrasing
        # DistilBART doesn't need a task prefix like T5
        inputs = tokenizer(text, return_tensors="pt", max_length=512, truncation=True, padding=True)
        
        # Generate attention mask for BART
        attention_mask = inputs.get('attention_mask', None)
        
        outputs = model.generate(
            inputs["input_ids"],
            attention_mask=attention_mask,
            max_length=len(text.split()) + 50,  # Allow some expansion
            min_length=max(5, len(text.split()) - 10),  # Don't make it too short
            length_penalty=2.0, 
            num_beams=4, 
            early_stopping=True,
            do_sample=True,
            temperature=0.8
        )
        
        rephrased = tokenizer.decode(outputs[0], skip_special_tokens=True)
        
        return jsonify({
            "original": text,
            "rephrased": rephrased
        })
    except Exception as e:
        logger.error(f"Error occurred in /api/rephrase: {str(e)}")
        return jsonify({"error": str(e)}), 500

if __name__ == '__main__':
    app.run(debug=False, host='0.0.0.0', port=5000, use_reloader=False)