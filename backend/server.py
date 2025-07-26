from rapidfuzz import fuzz, process # Fuzzy search library

from flask import Flask, jsonify, request # Server framework
from flask_cors import CORS # Enable CORS for Qt integration

from transformers import AutoTokenizer, AutoModelForSeq2SeqLM # Model for summarization
import logging

# Set up logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

app = Flask(__name__)
CORS(app)  # Enable CORS for all routes

# initialize the tokenizer and model for summarization
import os
model_path = os.path.join(os.path.dirname(os.path.dirname(__file__)), "models", "t5-small")
tokenizer = AutoTokenizer.from_pretrained(model_path)
model = AutoModelForSeq2SeqLM.from_pretrained(model_path)

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
        
        # Validate ratio
        if not isinstance(ratio, (int, float)) or ratio <= 0 or ratio > 1:
            return jsonify({
                "error": "Ratio must be a number between 0 and 1"
            }), 400
        
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

        # Calculate target summary length based on ratio
        original_word_count = len(text.split())
        target_length = max(10, int(original_word_count * ratio))  # Ensure minimum of 10 words
        max_length = min(target_length + 20, 150)  # Add buffer, cap at 150

        # Prepend task (required for T5-style models)
        input_text = "summarize: " + text

        inputs = tokenizer(input_text, return_tensors="pt", max_length=512, truncation=True)
        summary_ids = model.generate(
            inputs["input_ids"], 
            max_length=max_length, 
            min_length=max(5, target_length - 10), 
            length_penalty=2.0, 
            num_beams=4, 
            early_stopping=True
        )

        summary = tokenizer.decode(summary_ids[0], skip_special_tokens=True)
        
        return jsonify({
            "summary": summary,
            "original_length": len(text.split()),
            "summary_length": len(summary.split()),
            "compression_ratio": round(len(summary.split()) / len(text.split()), 3)
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
        
        # Use T5 model for paraphrasing
        input_text = "paraphrase: " + text
        
        inputs = tokenizer(input_text, return_tensors="pt", max_length=512, truncation=True)
        outputs = model.generate(
            inputs["input_ids"], 
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