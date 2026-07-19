# Vertical Slice Two-Phase Capture Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Separar captura visual e certificação para que reviews externas possam se vincular a um manifest imutável antes do gate final.

**Architecture:** Um módulo PowerShell isolado controla limpeza/captura/reuso e valida integralmente a identidade do manifest. O runner mantém o fluxo legado, adiciona dois parameter sets explícitos e só entra em reviews/evidência depois que o helper retorna uma captura validada.

**Tech Stack:** Windows PowerShell 5.1, módulos `.psm1`, contratos Python `unittest`, fixtures temporárias PowerShell.

## Global Constraints

- `-CaptureOnly` e `-UseExistingCapture` implicam o gate visual e são mutuamente exclusivos.
- O default permanece compatível com o fluxo atual.
- CaptureOnly não lê, cria, aprova ou modifica reviews e não escreve evidência.
- UseExistingCapture nunca limpa nem executa captura.
- Configuração, path, schema, conjunto de artefatos, hashes e goldens falham fechados.
- Nenhum commit, push, HTML ou recaptura visual completa faz parte desta execução.

---

### Task 1: Capture workflow helper

**Files:**
- Create: `tools/VerticalSliceCaptureWorkflow.psm1`
- Create: `tools/tests/vertical-slice-capture-workflow-tests.ps1`

**Interfaces:**
- Produces: `Invoke-NinhoVerticalSliceCaptureWorkflow -Root -Configuration -ArtifactDirectory -Mode [-CaptureOperation]`.
- Returns: `{ ManifestPath, Manifest, ManifestSha256 }` somente após validação completa.

- [ ] **Step 1: Write the failing fixture test**

Criar manifest e artefatos mínimos em temp root; exigir recaptura para Legacy/CaptureOnly, preservação para UseExistingCapture e falhas para configuração, path, arquivo, hash e golden inválidos.

- [ ] **Step 2: Run test to verify RED**

Run: `powershell -NoProfile -File tools/tests/vertical-slice-capture-workflow-tests.ps1 -Root <repo>`
Expected: FAIL porque `VerticalSliceCaptureWorkflow.psm1` não existe.

- [ ] **Step 3: Implement minimal helper**

Validar diretório canônico, limpar somente modos de captura, resolver o manifest canônico e verificar `source_artifacts`, `goldens` e `golden_metadata` com paths relativos seguros e hashes SHA-256 exatos.

- [ ] **Step 4: Run fixture GREEN**

Run: `powershell -NoProfile -File tools/tests/vertical-slice-capture-workflow-tests.ps1 -Root <repo>`
Expected: `vertical-slice-capture-workflow-tests: PASS`.

### Task 2: Two-phase CLI orchestration

**Files:**
- Modify: `tools/test_vertical_slice.ps1`
- Modify: `tools/tests/test_vertical_slice_tooling_contracts.py`

**Interfaces:**
- Consumes: `Invoke-NinhoVerticalSliceCaptureWorkflow` from Task 1.
- Produces: `-CaptureOnly`, `-UseExistingCapture`, marker `VERTICAL_SLICE_CAPTURE_READY` and legacy behavior.

- [ ] **Step 1: Add failing source-level CLI contracts**

Exigir parameter sets exclusivos, visual implícito, helper compartilhado, saída CaptureOnly antes de reviews e branch UseExisting sem captura direta.

- [ ] **Step 2: Run tooling RED**

Run: `python tools/tests/test_vertical_slice_tooling_contracts.py`
Expected: FAIL por switches/marker ausentes.

- [ ] **Step 3: Implement orchestration**

Selecionar modo pelo parameter set, preservar nonvisual default, chamar helper, sair em CaptureOnly e reutilizar o documento/hash retornados nos fluxos Legacy/UseExistingCapture.

- [ ] **Step 4: Run fixture and tooling GREEN**

Run both focused test commands from Tasks 1 and 2.

### Task 3: Operational documentation

**Files:**
- Modify: `README.md`
- Modify: `tools/tests/test_vertical_slice_tooling_contracts.py`

**Interfaces:**
- Produces: sequência documentada CaptureOnly → revisão externa → UseExistingCapture para Debug e Release.

- [ ] **Step 1: Add failing documentation contract**

Exigir os dois comandos e texto explícito de que CaptureOnly não certifica nem produz reviews.

- [ ] **Step 2: Run documentation RED**

Run: `python tools/tests/test_vertical_slice_tooling_contracts.py`
Expected: FAIL por comandos ausentes no README.

- [ ] **Step 3: Document exact commands and trust boundary**

Adicionar comandos PowerShell, marker usado para vinculação e avisos contra recaptura/review fabricada.

- [ ] **Step 4: Run final focused verification**

Run fixture workflow, tooling contracts, identity contract and vertical-slice gate fixture. Do not run the full visual capture.

## Plan Self-Review

- A spec inteira está coberta por Tasks 1–3.
- Não há placeholders nem APIs divergentes.
- O helper é a única fronteira que pode apagar/capturar; o runner só decide modo e certificação.
- A execução será inline, conforme instrução explícita do responsável, e sem commits.
