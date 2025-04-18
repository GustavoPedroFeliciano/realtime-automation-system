# Trabalho Final — Automação em Tempo Real (ELT127) - 2024/1

**Professor:** Luiz T. S. Mendes  
**Departamento:** Engenharia Eletrônica - UFMG  

---

## 🧠 Introdução

O trabalho propõe o desenvolvimento de um **sistema de controle multitarefa** para visualização de dados e alarmes no processo de **carregamento de vagões com minério de ferro**. O sistema simula a integração entre um CLP, uma balança inteligente e um sistema supervisório em um ambiente multithread com **sincronização, IPC, temporização e arquivos circulares**.

---

## 🎯 Objetivos

- Criar um sistema com múltiplas **threads/processos**, cada um representando uma funcionalidade distinta.
- Implementar **comunicação entre threads** (IPC) e **sincronização com eventos**.
- Criar estruturas como **lista circular em memória** e **arquivo circular em disco**.
- Simular alarmes e dados de processo, exibindo-os em terminais distintos.

---

## 🧩 Tarefas do Sistema

| Tarefa | Função |
|--------|--------|
| 1. Leitura da balança | Gera alarmes aleatórios (1–5s) e armazena em lista circular |
| 2. Leitura do CLP | Gera dados de processo (500ms) e alarmes (1–5s), armazena na mesma lista |
| 3. Captura de alarmes | Consome alarmes da balança e envia para exibição |
| 4. Captura de dados CLP | Consome dados/alarmes do CLP, grava dados em disco ou envia alarmes |
| 5. Exibição de alarmes | Recebe e exibe alarmes formatados |
| 6. Exibição de dados | Lê do arquivo circular e exibe dados formatados |
| 7. Leitura do teclado | Controla bloqueios/desbloqueios das tarefas e encerra o sistema |

---

## 🛠️ Estrutura Técnica

- **Comunicação**: Pipes ou Mailslots entre processos
- **Sincronização**: Objetos de evento (WinAPI)
- **Temporização**: Métodos customizados (sem `Sleep()`)
- **Threads/processos**: Criados via `CreateProcess()` no Visual Studio
- **Armazenamento**: Lista circular (memória) + arquivo circular `processo.txt`

---

## 🧪 Formato das Mensagens

### Alarmes (balança e CLP)
NNNNNN#OR#CD#HH:MM:SS Exemplo: "007745#00#23#12:43:05"

shell
Copy
Edit

### Dados de Processo (CLP)
NNNNNN#99#VEL#IC#FC#HH:MM:SS Exemplo: "105633#55#002.0#1#0#21:44:13"

yaml
Copy
Edit

---

## 🎮 Comandos do Teclado

| Tecla | Ação |
|-------|------|
| `b` | Liga/desliga leitura da balança |
| `c` | Liga/desliga leitura do CLP |
| `a` | Liga/desliga captura de alarmes |
| `d` | Liga/desliga captura de dados |
| `1` | Liga/desliga exibição de alarmes |
| `2` | Liga/desliga exibição de dados |
| `ESC` | Encerra todas as tarefas |

---

## 🔧 Ferramenta de Desenvolvimento

- **Microsoft Visual Studio Community Edition**
- Linguagem: **C/C++**
- Organização recomendada: **uma solução com múltiplos projetos**

---

> **Dica**: Teste tudo com cuidado e não deixe para última hora. Cada tarefa é inspirada em exemplos vistos em aula e no livro _"Programação Concorrente em Ambiente Windows"_.

