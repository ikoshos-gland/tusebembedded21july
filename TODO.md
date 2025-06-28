# 📋 Project TODO & Roadmap

This document enumerates actionable tasks required to improve, harden and extend the **sEMG Hand-Prosthesis** code-base.  
Please keep the check-boxes updated and append notes, decisions or links to pull-requests next to each item when progress is made.

Legend&nbsp;|&nbsp;Glyph  
:--|:--  
☐ | not&nbsp;started  
▣ | in-progress  
☑ | done  

<small>(Render on GitHub with `- [ ]`, `- [x]`, or `- [-]`.)</small>

---

## 1. Configuration & User-Experience

- [ ] **1.1 Schema Validation** – add JSONSchema/YAML-schema validation for [`user_config.yaml`](user_config.yaml:1).  
  **Acceptance criteria**: running `python -m common.validate_config user_config.yaml` prints “✔ valid” or errors with line + column; CI fails on invalid config.  
- [ ] **1.2 Interactive CLI Prompts** – extend [`cli.py`](cli.py:1) to ask for missing parameters (e.g. `general.gpu_memory_limit`) with sane defaults.  
- [ ] **1.3 Config Modularization** – split **user_config.yaml** into base + mode-specific overlays under `configs/` (e.g. `training.yaml`, `evaluation.yaml`). Update Hydra search-path accordingly.

## 2. Code Quality / Refactor

- [ ] **2.1 Remove `common` path hack** – migrate utilities from external *common/* repo into [`src/utils`](src/utils/__init__.py:1) and delete `sys.path` manipulation in [`stm32ai_main.py`](stm32ai_main.py:17-20).  
- [ ] **2.2 Type Hints & `__all__`** – add complete type annotations and explicit exports across `src/*` modules; enforce with `mypy --strict`.  
- [ ] **2.3 Extract CLI Entrypoint** – move argparse logic (lines 163-178) into new [`cli.py`](cli.py:1) keeping [`stm32ai_main.py`](stm32ai_main.py:1) import-safe.  
- [ ] **2.4 Header Consolidation** – unify duplicate struct definitions in `src/*.h` into `src/include/shared_types.h`; include from each module.  
- [ ] **2.5 Logging Framework** – replace `print()` with Python `logging` in libs and with `SEGGER_RTT_printf` on firmware; add `--verbosity` flag.

## 3. Testing

- [ ] **3.1 Unit Tests (Python)** – pytest coverage ≥ 80 % for `src/preprocessing`, `src/utils/parse_config.py`, `src/models/random_forest_emg.py`.  
- [ ] **3.2 End-to-End Smoke Test** – workflow: preprocess → 1-epoch train → evaluate; run nightly in CI, max 10 min.  
- [ ] **3.3 Firmware Unit Tests** – integrate Ceedling/Unity; mock HAL to test `DSP_pipeline()` and `RandomForest_predict()`.  
- [ ] **3.4 Continuous Benchmark** – run [`benchmark`](src/benchmarking/README.md:1) with STM32Cube.AI CLI in CI; compare ‑-fps against threshold.

## 4. Documentation

- [ ] **4.1 README ToC Fix** – regenerate Table-of-Contents links (lines 26-342).  
- [ ] **4.2 Sphinx Docs** – auto-generate API docs from source, host on GitHub Pages.  
- [ ] **4.3 Tutorial Notebook** – step-by-step introduction in `notebooks/getting_started.ipynb`.  
- [ ] **4.4 Add LICENSE** – choose a permissive OSI license (MIT/BSD-3 or Apache-2).  

## 5. Automation & CI/CD

- [ ] **5.1 GitHub Actions** – workflow: lint → mypy → pytest → build-firmware → upload artifacts.  
- [ ] **5.2 Experiment Tracking Stack** – docker-compose file spinning MLflow server & ClearML agent; documented in `deployment/README.md`.  
- [ ] **5.3 Cache STM32Cube.AI** – pre-install and cache ~1 GB SDK to cut CI time.

## 6. Performance & Reliability

- [ ] **6.1 tf.data Optimizations** – add `.prefetch(tf.data.AUTOTUNE)` & `num_parallel_calls=AUTOTUNE`.  
- [ ] **6.2 GPU Memory Check** – verify TensorFlow respects limit; raise warning otherwise.  
- [ ] **6.3 Firmware Watchdog** – enable IWDG; reset on classification stall > 1 s.  
- [ ] **6.4 CMSIS-NN TFLite-Micro** – benchmark vs Random-Forest for latency & accuracy.

## 7. Feature Roadmap

- [ ] **7.1 Servo-Angle Regression** – build TCN model predicting continuous angle.  
- [ ] **7.2 QAT Pipeline** – add quantization-aware training flag in [`CNN2D_ST_HandPosture.py`](src/models/CNN2D_ST_HandPosture.py:1).  
- [ ] **7.3 BLE Streaming App** – mobile Flutter demo visualizing live predictions.

## 8. Dataset Management

- [ ] **8.1 DVC Integration** – track `datasets/` and `pretrained_models/`.  
- [ ] **8.2 Data Anonymization Script** – scrub subject identifiers from raw sEMG CSV.  
- [ ] **8.3 Acquisition Protocol Docs** – enrich `datasets/README.md` with electrode placement, sampling rate, ethics.

## 9. Security & Compliance

- [ ] **9.1 Dependency Pinning** – generate `requirements.lock` via pip-tools.  
- [ ] **9.2 Static Analysis** – add `bandit -r .` and `safety check` to CI.  
- [ ] **9.3 License Audit** – ensure STM32Cube.AI redistribution terms allow binary artifacts; document findings.

---

_Last generated: 2025-06-28_