# vib-ribbon-recomp

Experimento de recompilação estática para PC de **Vib-Ribbon** (PlayStation, PAL, `SCES-02873`) usando o [PSXRecomp](https://github.com/RetroPortingToolKit/psxrecomp). O objetivo é estudar a tradução dos módulos do jogo para C, selecioná-los com overlays AOT e ligá-los a um runtime que roda a partir do próprio disco do jogador.

> Este repositório **não contém** ROM, discos, trilha sonora, texturas ou qualquer outro conteúdo protegido de Vib-Ribbon. Você precisa fornecer um dump legal do seu próprio exemplar. Os hashes em `analysis/` são apenas identificadores usados para validar esse disco localmente.

## Estado atual

- A fase inicial de geração isolada produziu 79, 578, 522 e 924 entradas de dispatch para `SCES_028.73`, `MAIN_T.EXE`, `MAIN_K.EXE` e `MAIN_G.EXE` — 46 objetos compilados, sem link nem execução. Detalhes em `analysis/recomp-experiment.json`.
- `aot/overlays.json` descreve os três módulos sobrepostos, selecionados pela verificação dos bytes residentes em vez de fundir diretamente os dispatches.
- `psx_last_run_report.json` guarda o último relatório local conhecido: build `v0.3.4-alpha-25-gd35f13c9-dirty`, 8.877 frames, 107.016 dispatches nativos e 13 quedas para o interpretador, com 97 overlays registrados/validados. É telemetria de uma sessão encerrada pelo fechamento da janela SDL — **não** é prova de conclusão do jogo nem de paridade com o original.
- O executável gerado localmente fica fora do Git. Ele pode ser anexado a um Release separado; o que está versionado aqui é o código, a configuração e a documentação.

## Estrutura

| Caminho | Conteúdo |
|---|---|
| `game.toml` | Configuração principal: serial, execução de boot, seeds, runtime e digests do disco. |
| `config/`, `seeds/` | Configurações e seeds isolados de título, tutorial e jogo. |
| `aot/overlays.json` | Perfil AOT dos três executáveis sobrepostos. |
| `analysis/` | Inventário do disco, dos executáveis e dos relatórios de engenharia reversa. |
| `tools/` | Scripts de análise, geração de código, checagem e build do runtime. |
| `bios/openbios.bin` | BIOS livre do PCSX-Redux usada pelo runtime; licença em `bios/OpenBIOS.LICENSE`. |
| `patches/` | Ajustes aplicados ao checkout local do PSXRecomp. |
| `mods/` | Manifests de enhancement de exemplo. |
| `psx_last_run_report.json`, `VibRibbon_Recompiled.execution.json` | Evidências de execução/telemetria local. |

## Como reproduzir (Windows)

1. Instale CMake 3.24+, Ninja, Clang/Clang++ 22.x (testado com 22.1.8), Python 3.11+, Node.js e Git. Os scripts esperam o Ninja em `.local/build-tools/bin/ninja.exe`.
2. Clone o PSXRecomp em `tools/psxrecomp-src`, na revisão `a7b37e079e8c6e98e04cd1cd42ba559e95911ea7`. O checkout é ignorado pelo Git.
3. Com o seu dump legal, coloque o CUE/BIN em `disc/` e extraia `SCES_028.73`, `MAIN_T.EXE`, `MAIN_K.EXE` e `MAIN_G.EXE` em `.local/executables/`. Ajuste `game.toml` se o nome do CUE for diferente.
4. Execute, na raiz do repositório:

```powershell
pwsh tools/build-analyzer.ps1
pwsh tools/analyze-executables.ps1
py -3 tools/prepare-recomp.py
pwsh tools/build-recompiler.ps1
pwsh tools/generate-recomp.ps1
pwsh tools/check-recomp.ps1
pwsh tools/build-runtime.ps1 -Python <caminho\para\python.exe>
```

O runtime resultante lê `game.toml` e exige o disco configurado. Saídas intermediárias ficam em `.local/`, ignorado pelo Git.

## Limitações conhecidas

- O suporte a MP3/streaming de áudio ainda não foi implementado neste experimento.
- Quatro seeds internos do carregador não têm função hospedeira e caem no interpretador.
- A compilação dos objetos não estabelece equivalência semântica; não há suíte de paridade com o jogo original.
- O fluxo foi validado apenas em Windows com a toolchain descrita acima.

## Licença e créditos

O código e a configuração deste repositório são MIT — veja `LICENSE`. Componentes de terceiros e avisos de direitos autorais estão em `THIRD_PARTY_NOTICES.md`. Vib-Ribbon pertence aos seus respectivos detentores; este projeto não é afiliado a eles.

Leitura complementar: `analysis/ESTUDO.md` (estudo estático inicial) e `analysis/RECOMPILACAO.md` (primeiro experimento com o framework). O mecanismo de AOT usado está documentado em [AOT_SHARDING.md](https://github.com/RetroPortingToolKit/psxrecomp/blob/master/docs/AOT_SHARDING.md).
