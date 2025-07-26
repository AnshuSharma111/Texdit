# Texdit - AI-Powered Text Manipulation Tool

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey.svg)
![Qt](https://img.shields.io/badge/Qt-6.9-green.svg)
![Python](https://img.shields.io/badge/Python-3.8%2B-blue.svg)

## Overview

**Texdit** is an innovative, offline AI-powered text manipulation tool that transforms how you work with text. Built with Qt/C++ frontend and Python backend, it provides powerful text processing capabilities through an intuitive command-driven interface.

## ✨ Key Features

### 🧠 AI-Powered Text Processing
- **Smart Summarization** - Condense long texts while preserving key information
- **Keyword Extraction** - Automatically identify and highlight important terms
- **Tone Analysis & Modification** - Analyze and adjust the tone of your text
- **Intelligent Rephrasing** - Rewrite content while maintaining meaning
- **Context-Aware Processing** - Uses T5 transformer model for high-quality results

### ⚡ Powerful Command System
- **Fuzzy Search Commands** - Type partial commands and get intelligent suggestions
- **Real-time Feedback** - Instant visual feedback on command execution
- **Arrow Key Navigation** - Navigate through command suggestions effortlessly
- **Auto-completion** - Smart command completion with Tab key
- **Command Validation** - Real-time validation with visual indicators

### 🔒 Privacy-First Design
- **100% Offline Operation** - All AI processing happens locally
- **No Data Transmission** - Your text never leaves your machine
- **Secure Processing** - Complete privacy and data security
- **No Internet Required** - Works without any network connection

### 🎨 Modern User Experience
- **Clean Interface** - Minimalist design focused on productivity
- **Professional Loading Screen** - Smooth startup experience
- **Real-time Status Updates** - Always know what's happening
- **Responsive Design** - Adapts to your workflow
- **Keyboard Shortcuts** - Ctrl+/ to focus command input

## 🚀 Available Commands

| Command | Description | Example Usage |
|---------|-------------|---------------|
| `summarise` | Create concise summaries of long text | Perfect for articles, reports, documents |
| `keywords` | Extract key phrases and important terms | Identify main topics and concepts |
| `tone` | Analyze the emotional tone of text | Understand sentiment and mood |
| `rephrase` | Rewrite text with different phrasing | Improve clarity and style |
| `help` | Show available commands and usage | Get started quickly |
| `clear` | Clear the text input area | Start fresh |

## 🛠️ Technology Stack

### Frontend (Qt/C++)
- **Qt 6.9** - Modern cross-platform UI framework
- **Robust Architecture** - ServerManager, CommandManager, LoadingScreen
- **Event-Driven Design** - Responsive and efficient
- **Memory Safety** - QPointer-based object lifecycle management

### Backend (Python)
- **Flask** - Lightweight web framework for API
- **Transformers** - Hugging Face library for AI models
- **T5-Small Model** - Optimized transformer for text processing (233MB)
- **RapidFuzz** - High-performance fuzzy string matching
- **TensorFlow** - Machine learning framework

### AI Model
- **T5-Small** - Text-to-Text Transfer Transformer
- **Optimized Size** - Reduced from 2.2GB to 233MB
- **Local Processing** - No cloud dependencies
- **High Quality** - State-of-the-art results for text tasks

## 📋 Requirements

### System Requirements
- **Operating System**: Windows 10/11, macOS 10.15+, or Linux
- **RAM**: 4GB minimum, 8GB recommended
- **Storage**: 1GB free space for models and application
- **Processor**: Any modern CPU (no GPU required)

### Development Requirements
- **Qt 6.9+** with MinGW (Windows) or GCC (Linux/macOS)
- **Python 3.8+** with pip
- **CMake 3.16+** for building
- **Git** for version control

## 🔧 Installation & Setup

### 1. Clone the Repository
```bash
git clone https://github.com/AnshuSharma111/Texdit.git
cd Texdit
```

### 2. Install Python Dependencies
```bash
cd backend
pip install -r requirements.txt
```

### 3. Build the Application
```bash
mkdir build && cd build
cmake ..
cmake --build .
```

### 4. Run Texdit
```bash
./texdit  # Linux/macOS
texdit.exe  # Windows
```

## 🎯 Getting Started

1. **Launch Texdit** - The application will automatically start the AI backend
2. **Enter Text** - Type or paste your text in the main area
3. **Use Commands** - Press `Ctrl+/` to focus the command input
4. **Get Suggestions** - Start typing a command to see fuzzy-matched suggestions
5. **Execute Commands** - Press Enter or click Execute to process your text
6. **View Results** - See the processed output appended to your text

### Example Workflow
```
1. Paste a long article into the text area
2. Press Ctrl+/ to focus command input
3. Type "sum" and select "summarise" from suggestions
4. Press Enter to generate a concise summary
5. Type "key" and select "keywords" to extract key terms
```

## 🏗️ Architecture

### Component Overview
```
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│   MainWindow    │◄──►│  ServerManager   │◄──►│ Python Backend  │
│  (Qt/C++ UI)   │    │ (Health Monitor) │    │ (Flask + AI)    │
└─────────────────┘    └──────────────────┘    └─────────────────┘
         ▲                        ▲                       ▲
         │                        │                       │
         ▼                        ▼                       ▼
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│ CommandManager  │    │  LoadingScreen   │    │   T5 Model      │
│ (Validation)    │    │ (Startup UX)     │    │ (AI Processing) │
└─────────────────┘    └──────────────────┘    └─────────────────┘
```

### Key Design Principles
- **Separation of Concerns** - Each component has a single responsibility
- **Robust Error Handling** - Graceful degradation and clear error messages
- **Async Processing** - Non-blocking UI with background AI processing
- **Resource Management** - Efficient memory and process management

## 🤝 Contributing

We welcome contributions! Here's how you can help:

### Development Areas
- **New AI Commands** - Add more text processing capabilities
- **UI/UX Improvements** - Enhance the user experience
- **Performance Optimization** - Make processing faster and more efficient
- **Platform Support** - Improve cross-platform compatibility
- **Documentation** - Help others understand and use Texdit

### Getting Started with Development
1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Make your changes
4. Test thoroughly
5. Commit your changes (`git commit -m 'Add amazing feature'`)
6. Push to the branch (`git push origin feature/amazing-feature`)
7. Open a Pull Request

## 📝 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- **Hugging Face** - For the T5 model and transformers library
- **Qt Project** - For the excellent cross-platform framework
- **Flask Team** - For the lightweight web framework
- **Open Source Community** - For the countless libraries that make this possible

## 🔮 Roadmap

### Upcoming Features
- [ ] **Custom Model Support** - Load your own fine-tuned models
- [ ] **Batch Processing** - Process multiple files at once
- [ ] **Export Options** - Save results in various formats
- [ ] **Themes & Customization** - Personalize your workspace
- [ ] **Plugin System** - Extend functionality with plugins
- [ ] **Language Support** - Multi-language text processing

### Long-term Vision
- Advanced text analytics and insights
- Integration with popular text editors
- Cloud sync for settings (while keeping processing local)
- Mobile companion app
- Enterprise features for team collaboration

---

**Made with ❤️ by the Texdit Team**

*Transform your text, enhance your productivity, protect your privacy.*
