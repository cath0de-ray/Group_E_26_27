# Project VITALIS: AI-Driven Stethoscope Attachment
**Versatile Intelligent Thoracic Acoustic Lung Inference System**

![Project Status](https://img.shields.io/badge/Status-Prototype_Development-blue)
![AI Accuracy](https://img.shields.io/badge/AI_Accuracy-80.7%25-green)
![Grade](https://img.shields.io/badge/Academic_Level-12th_Grade-purple)
![License](https://img.shields.io/badge/License-CC_BY--NC--SA_4.0-lightgrey)

## 👥 Development Team
* **Pradhumya Singh Yadav**
* **Dhairya Chaudhary**
* **Aryan Chauhan**
---
## Data Collection GoogleSheet

 https://docs.google.com/spreadsheets/d/1PQ8IrSCbB4RT1YasqXPo0PYN3hZgyODFfWerC2Cv7SQ/edit?usp=sharing

---
## 🩺 Executive Summary
**VITALIS** (Versatile Intelligent Thoracic Acoustic Lung Inference System) is an low-cost, offline AI diagnostic tool. It is engineered as a attachment that fits onto standard analog stethoscope. 

By capturing acoustic pulmonary data and processing it locally via Edge-AI, VITALIS identifies respiratory anomalies (Wheezes, Crackles, Rhonchi) in real-time. It is specifically designed to empower NGO field workers and frontline health staff in resource-depleted environments to perform rapid medical identification without requiring an internet connection or an on-site pulmonologist.

---
## 5W1H
<img width="429" height="475" alt="5w1h" src="https://github.com/user-attachments/assets/a906d8c5-4656-48b1-bdde-556384406ec9" />


---

## 📂 Hardware Support & Versions
Project VITALIS currently supports two different microcontroller architectures. Please navigate to the appropriate folder for your specific hardware:

* **[Hardware V2 (XIAO ESP32-C6)](/Arduino%20Code/XIAO%20ESP32C6):**
* **[Hardware V1 (Classic ESP32)](/Arduino%20Code/ESP-32-WROOM):**
---

## 🌍 Alignment with UN Sustainable Development Goals (SDGs)
This project is purpose-built to address the "Last Mile" healthcare gap, actively targeting three primary UN SDGs:

### 1. SDG 3: Good Health and Well-being
* **The Problem:** Respiratory diseases are leading causes of mortality in rural areas due to misdiagnosis.
* **The VITALIS Solution:** Providing an automated "second opinion" for lung disease for early detection and timely intervention.

### 2. SDG 9: Industry, Innovation, and Infrastructure
* **The Problem:** Commercial digital stethoscopes are prohibitively expensive
* **The VITALIS Solution:** Utilizes "Edge Computing" via neural networks to provide diagnostic infrastructure with zero INTERNET dependency.
---

## ⚙️ System Architecture & Workflow
VITALIS operates on a completely localized, closed-loop pipeline ensuring 100% patient data privacy.

1. **Acoustic Listening:** Analog lung sounds are captured via the stethoscope chest-piece and routed to INMP441 microphone sensor.
2. **Digital Conversion:** The **INMP441 I2S MEMS Microphone** samples the audio at high precesion, converting acoustic sound into digital I2S data.
3. **DSP & Feature Extraction:** The MCU buffers audio in 1000ms windows and calculates Mel Frequency Cepstral Coefficients (MFCC) to seperate the sound types.
4. **AI Inference:** The extracted features are fed into the neural network Trained on EDGE Impulse.
5. **Output UI:** The predicted respiratory state is displayed on the **Mobile App** through BLE (Bluetooth Low Energy).

---

## 🧠 Neural Network Performance
The core engine was trained via Edge Impulse on a clinically verified dataset and cross-profiled for both Xtensa and RISC-V architectures.

### Validation Metrics
* **Overall Accuracy:** 80.7%
* **Overall F1 Score:** 0.80
* **Training Depth:** 600 Epochs
* **Optimization:** Quantized (int8) to operate within the SRAM constraints of the ESP32 ecosystem.

### Evidence of Training & Hardware Optimization

#### **Hardware V2: Seeed Studio XIAO ESP32-C6 (160MHz RISC-V)**
![XIAO C6 Accuracy Report](accuracy_v2_c6.png)

#### **Hardware V1: Classic ESP32 (240MHz Xtensa)**
![Classic ESP32 Accuracy Report](accuracy_report.png)

---

## 🛠️ Hardware Specifications

### **Prototype A**
* **MCU:** Seeed Studio XIAO ESP32-C6 (Single-Core RISC-V @ 160MHz)
* **Acoustic Sensor:** INMP441 I2S Digital MEMS Microphone

### **Prototype B**
* **MCU:** ESP32 DevKit V1 (Dual-Core Xtensa LX6 @ 240MHz)
* **Acoustic Sensor:** INMP441 I2S Digital MEMS Microphone

---

## ⚖️ Intellectual Property & Licensing

### **Non-Commercial License**
This project is licensed under the **Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0)**.
Under this license, others may share and adapt this material with proper **Attribution**, ensuring **Non-Commercial** use, and distributing modifications under the same **ShareAlike** license.

### **Patent & Prior Art Notice**
The Lead Developer reserves the right to file for **Patent Protection** regarding the unique hardware integration, acoustic coupling mechanism, and neural network architecture. This public disclosure serves as **Prior Art** under international patent law.

---

## 📂 Dataset Attribution
The AI model was trained using the HLS-CMDS dataset.
> **Citation:** Y. Torabi, S. Shirani and J. P. Reilly, "Descriptor: Heart and Lung Sounds Dataset Recorded from a Clinical Manikin using Digital Stethoscope (HLS-CMDS)," in IEEE Data Descriptions, 2025. © 2024 Yasaman Torabi.

---
**Institution:** Jagran Public School, Noida  
**Submission:** CBSE Class 12 Artificial Intelligence Practical (2026-27)

``` 
---
config:
  layout: dagre
---
graph TD
    classDef terminator fill:#000,stroke:#f8fafc,stroke-width:2px,color:#f8fafc;
    classDef io fill:#0a2a5e,stroke:#0ea5e9,stroke-width:2px,color:#fff;
    classDef process fill:#0f172a,stroke:#38bdf8,stroke-width:2px,color:#fff;
    classDef decision fill:#1e293b,stroke:#00d1b2,stroke-width:3px,color:#fff;
    START([Start: Device Activation]):::terminator

    N1[/Input: INMP441 Captures Audio Data/]:::io
    
    subgraph Pre-Processing
        N2[Process: I2S Digital Conversion]:::process
        N3[Process: Feature Extraction]:::process
    end
    D1{Is it Normal Lungs?}:::decision
    D2{Is it Wheeze?}:::decision
    D3{Is it Crackles?}:::decision
    D4{Is it Rhonchi?}:::decision
    OUT_NORMAL[\Output: Send 'Normal' to Mobile App\]:::io
    OUT_WHEEZE[\Output: Send 'Wheeze Detected' to Mobile App\]:::io
    OUT_CRACKLE[\Output: Send 'Crackles Detected' to Mobile App\]:::io
    OUT_RHONCHI[\Output: Send 'Rhonchi Detected' to Mobile App\]:::io
    OUT_UNKNOWN[\Output: Send 'Unrecognized Sound' to Mobile App\]:::io

    END([Stop: Diagnostic Finished]):::terminator
    START --> N1
    N1 --> N2
    N2 --> N3
    N3 --> D1
    
    D1 -- Yes --> OUT_NORMAL
    D1 -- No --> D2
    
    D2 -- Yes --> OUT_WHEEZE
    D2 -- No --> D3
    
    D3 -- Yes --> OUT_CRACKLE
    D3 -- No --> D4
    
    D4 -- Yes --> OUT_RHONCHI
    D4 -- No --> OUT_UNKNOWN
    OUT_NORMAL --> END
    OUT_WHEEZE --> END
    OUT_CRACKLE --> END
    OUT_RHONCHI --> END
    OUT_UNKNOWN --> END 
    ```