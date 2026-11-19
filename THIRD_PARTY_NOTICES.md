# Avisos de terceiros

Este arquivo cobre componentes citados ou usados pelo experimento. A licença MIT em `LICENSE` se aplica somente ao código e à configuração escritos neste repositório.

## PSXRecomp

- Projeto: RetroPortingToolKit/psxrecomp — https://github.com/RetroPortingToolKit/psxrecomp
- Licença: **PolyForm Noncommercial License 1.0.0**, copyright (c) 2026 Matthew Stanley.
- Revisão usada nos experimentos: `a7b37e079e8c6e98e04cd1cd42ba559e95911ea7`.
- O checkout fica em `tools/psxrecomp-src/` e é ignorado pelo Git. Código gerado pelo recompilador e pelo pipeline de overlays herda as condições do upstream.
- Referência de AOT: https://github.com/RetroPortingToolKit/psxrecomp/blob/master/docs/AOT_SHARDING.md

## OpenBIOS (PCSX-Redux)

- Arquivo redistribuído: `bios/openbios.bin`.
- Origem: https://github.com/grumpycoders/pcsx-redux (src/mips/openbios).
- Licença MIT, copyright (c) 2019 PCSX-Redux authors; o binário também incorpora código do uC-sdk sob licenças permissivas. O aviso completo está em `bios/OpenBIOS.LICENSE`.
- É uma BIOS livre, usada apenas para que o runtime gere e valide o código do jogo sem BIOS proprietária.

## Open Ribbon

- Projeto de referência consultado durante o estudo do formato dos arquivos do jogo.
- A documentação local em `tools/open-ribbon-docs/` está sob **CC0 1.0 Universal**, conforme `tools/open-ribbon-docs/LICENSE`. Esse checkout é ignorado pelo Git e não é redistribuído aqui.

## Vib-Ribbon

- O jogo original, sua música, arte, marcas e executáveis pertencem aos respectivos detentores de direitos. Nenhum desses conteúdos é redistribuído neste repositório.
- Os digests em `game.toml` e em `analysis/` identificam o disco PAL `SCES-02873` e existem somente para validar localmente um dump de propriedade do usuário.

## Dependências de build

Clang/LLVM, CMake, Ninja, SDL3, libchdr, libjuice, stb_image, xxhash, json11 e as demais bibliotecas obtidas pelo checkout do PSXRecomp permanecem sob suas próprias licenças e não são incorporadas a este repositório. O executável resultante do build deve ser acompanhado dos avisos aplicáveis quando distribuído.
