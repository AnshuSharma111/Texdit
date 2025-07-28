# TexDit Architecture Testing Scripts

This directory contains scripts used for testing and comparing different AI model architectures for TexDit.

## Files

### Core Testing Scripts
- `architecture_test.py` - Comprehensive offline testing of different model architectures
- `test_manager.py` - Server management utility for running performance comparisons
- `packaging_analysis.py` - Analysis of package sizes for different model configurations
- `quick_test.py` - Quick performance comparison between running servers

### Server Implementations
- `server.py` - Current production server (DistilBART + rule-based)
- `single_llm_server.py` - Test server for single LLM approach
- `download_model.py` - Utility for downloading and setting up AI models

## Usage

### Quick Performance Test
```bash
# Terminal 1: Start both test servers
python test_manager.py

# This will automatically:
# 1. Start the current architecture server (port 5000)
# 2. Start the single LLM test server (port 5001) 
# 3. Run performance comparison
# 4. Generate results and recommendations
```

### Packaging Analysis
```bash
python packaging_analysis.py
```

### Manual Testing
```bash
# Start individual servers
python server.py                    # Current architecture (port 5000)
python single_llm_server.py        # Single LLM test (port 5001)

# Run comparison
python quick_test.py
```

## Test Results Summary

Based on comprehensive testing (see `../ARCHITECTURE_DECISION_REPORT.md`):

- **Single LLM is 2-3x slower** than specialized models
- **Current architecture wins** on performance and user experience
- **Package size trade-off** is acceptable for better performance
- **Recommendation**: Continue with specialized models approach

## Model Requirements

### Current Architecture (Recommended)
- DistilBART-CNN-12-6: 1.14GB (summarization & rephrasing)
- Rule-based algorithms: 0GB (keywords & tone)
- Total: ~1.14GB for core features

### Tested Single LLM Options
- Flan-T5-Base: 990MB (too slow)
- Flan-T5-Large: 3GB (very slow)
- Phi-3-Mini: 3.8GB (untested, expected slow)

## Performance Thresholds
- **Excellent**: < 3 seconds per query
- **Acceptable**: 3-5 seconds per query  
- **Poor**: 5-10 seconds per query
- **Unacceptable**: > 10 seconds per query
