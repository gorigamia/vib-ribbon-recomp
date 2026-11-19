# Primeiro experimento com PSXRecomp

Data: 1 de novembro de 2026. Revisão do framework: a7b37e079e8c6e98e04cd1cd42ba559e95911ea7.

Este documento atualiza o estado posterior ao estudo estático em ESTUDO.md.

## Resultado confirmado

- Compilação das ferramentas psxrecomp-game, psxrecomp-toml e psxrecomp-bios com Clang 22.1.8.
- Probe oficial reconheceu SCES-02873, oito faixas e 83 seeds iniciais, sem avisos de identificação.
- Tradução dos quatro executáveis locais para C.
- Compilação isolada dos arquivos C para 46 objetos PC, sem erros de compilação.

| Módulo | Entradas de dispatch emitidas | Objetos compilados |
|---|---:|---:|
| SCES_028.73 | 79 | 3 |
| MAIN_T.EXE | 578 | 12 |
| MAIN_K.EXE | 522 | 11 |
| MAIN_G.EXE | 924 | 20 |

Os resultados estruturados estão em recomp-experiment.json. Os BIN/CUE originais foram somente lidos. C e objetos derivados ficam em .local, ignorado pelo Git.

## Limites observados

A primeira varredura do carregador confundiu uma string de dados com instruções, emitindo um aviso de opcode reservado. A configuração por caminhos alcançáveis removeu esse candidato. Quatro seeds interiores do carregador continuam sem função hospedeira e foram deixadas pela ferramenta para fallback de interpretação.

O gerador não registrou avisos na tradução dos outros três módulos nesta configuração. Isso não estabelece cobertura completa. O compilador também registrou avisos no C gerado, como uma comparação de registro consigo mesmo no módulo de título; eles estão preservados em .local/objects.

Compilar objetos demonstra que a saída é aceita pelo compilador local. Não demonstra equivalência semântica nem funcionamento do jogo. Nenhum executável final foi ligado ao runtime, nenhuma cena foi executada e MP3 ainda não foi implementado.

## Integração seguinte

Os três módulos maiores se sobrepõem a partir de 0x80019000 e geram nomes de função coincidentes. As saídas deste experimento são deliberadamente isoladas. Não devemos juntar diretamente seus arquivos de dispatch em um executável.

O próximo experimento deve usar o mecanismo de overlays com verificação dos bytes residentes, fornecido pelo framework, para selecionar o código correspondente ao módulo carregado. Depois disso: ligar o runtime, abrir o disco CUE completo e verificar boot → título → menu → primeira música original.

Referência do mecanismo: [AOT sharding do PSXRecomp](https://github.com/RetroPortingToolKit/psxrecomp/blob/master/docs/AOT_SHARDING.md). Os scripts reproduzíveis estão documentados no README.
