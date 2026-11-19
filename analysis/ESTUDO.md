# Estudo da recompilação do Vib-Ribbon para PC

Data: 1 de novembro de 2026. Disco europeu SCES_028.73.

> Documento histórico da primeira fase. O estado atual do experimento está no [README](../README.md) e em `RECOMPILACAO.md`.

## Resultado e limites

Os quatro executáveis foram extraídos e analisados estaticamente. Os 13 PAKs foram extraídos com validação de limites: 1.707 entradas, incluindo cópias por idioma. Os leitores de recursos interpretam a geometria TMD e as tabelas do script de fases.

Compilamos e executamos o **analisador** do PSXRecomp. Não compilamos o jogo ou seu runtime, não executamos cenas e ainda não existe build PC jogável. Esta etapa entrega evidências e ferramentas para orientar esse experimento.

## Módulos do jogo

| Executável | Papel | Carga na RAM | Entrada | GP configurado no código |
|---|---|---|---|---|
| SCES_028.73 | Carregador | 0x80010000 | 0x80011F2C | 0x800167CC |
| MAIN_T.EXE | Título e idioma | 0x80019000 | 0x8002BF78 | 0x80047EB8 |
| MAIN_K.EXE | Tutorial | 0x80019000 | 0x800294B4 | 0x80045418 |
| MAIN_G.EXE | Menus e partida | 0x80019000 | 0x8005405C | 0x80075E24 |

Endereços confirmados nos cabeçalhos e instruções locais; papéis corroborados pela [documentação do Open Ribbon](https://github.com/open-ribbon/documentation/blob/main/docs/decompilation/general-info.md).

Os três módulos maiores ocupam a mesma base de RAM. Precisamos identificar qual módulo está residente para escolher a função recompilada correta. O GP inicial do cabeçalho é zero, mas cada módulo o redefine no código; considerar apenas o cabeçalho prejudicaria a interpretação de variáveis globais.

O SHA-1 do MAIN_T local é `7eb79e31da2c7456a1ece52835948b75979ce65a`, exatamente o publicado no arquivo `sces028.sha` do [Open Ribbon](https://github.com/open-ribbon/open-ribbon). Os símbolos públicos da tela inicial podem ajudar com uma correspondência binária concreta. Eles não devem ser aplicados por endereço aos outros módulos.

## Gráficos

O pacote inglês GAME contém 31 modelos TMD. Foram verificados os limites de tabelas e primitivas e todos os índices das linhas decodificadas. Nesse conjunto há 6.489 primitivas de modo 0x40 e 19 de modo 0x21. A ferramenta decodifica a geometria das linhas 0x40; ainda não interpreta as primitivas 0x21.

| Recurso | Objetos | Vértices | Primitivas |
|---|---:|---:|---:|
| CHARA/PEELOO/MODEL.TMD | 16 | 156 | 176 |
| CHARA/FROG/MODEL.TMD | 14 | 148 | 159 |
| ROAD/ROAD.TMD | 10 | 103 | 93 |
| FONT/FE_FONT.TMD | 113 | 690 | 511 |

A prévia `.local/road-geometry.png` foi desenhada dos vértices e índices originais de ROAD/ROAD.TMD, que está no plano Y/Z. Usa projeção ortográfica e escala independente por objeto: demonstra a leitura dos obstáculos, não a reprodução da câmera, animações ou rasterização do jogo.

O visual simples ajuda, mas o jogo usa modelos e transformações. Os ANM animam objetos e os ANC animam a câmera. Para o primeiro boot, a recomendação é conservar os comandos de GPU/GTE pelo runtime; um renderizador próprio pode ser estudado depois de existir uma referência funcional. Referências: [TMD do Open Ribbon](https://github.com/open-ribbon/documentation/blob/main/docs/file-types/tmd.md), [GPU do PS1](https://psx-spx.consoledev.net/graphicsprocessingunitgpu/).

## Pacotes

Há seis PAKs GAME com 242 entradas cada, seis KIOSK com 36 cada e um TITLE com 39. São dados sem compressão no nível do arquivo PAK. A estrutura é contagem, tabela de offsets, nome terminado em zero e alinhado a quatro bytes, tamanho e conteúdo. O extrator verifica comprimentos, offsets e caminhos antes de escrever em `.local/resources`.

Os prefixos documentados 01/02/04/08/10/20 correspondem a japonês/inglês/alemão/espanhol/francês/italiano; não são seis fases diferentes. [Documentação de PAK](https://github.com/open-ribbon/documentation/blob/main/docs/file-types/pak.md).

## Geração de obstáculos

O SCRIPT/SYSTEM3.FSL local tem 26.920 bytes e o mesmo SHA-256 nos seis pacotes GAME. O leitor percorreu o arquivo até seu último byte, validando índices e ordenação de segmentos.

| Estrutura | Quantidade / início |
|---|---|
| Padrões fixos | 11 / 0x40 |
| Distribuições | 63 / 0xEC |
| Tabelas de controle | 7 / 0xDB8 |
| Tabelas de padrões | 7 / 0x56CC |
| Tabelas de eventos | 7 / 0x5EC8 |
| Índices de faixas | 7 / 0x68D4 |

O sétimo conjunto contém 774 segmentos de controle, 90 de padrões e 82 de eventos. A [pesquisa de FSL do Open Ribbon](https://github.com/open-ribbon/documentation/blob/main/docs/file-types/fsl.md) identifica esse conjunto como a base de CDs personalizados e descreve a escolha de posições iniciais nele.

Isso corrige uma hipótese anterior: **não está demonstrado que o jogo converta cada batida diretamente em obstáculos**. Já confirmamos a existência de padrões e scripts. Ainda precisamos rastrear a influência do áudio na seleção, no tempo e nas pausas. Preservar o algoritmo exige entender essa combinação.

Encontramos uma divergência na documentação comunitária: o primeiro campo das distribuições não funciona como uma contagem simples. Os bytes locais incluem vários valores diferentes de 1. Nosso leitor preserva esse campo como opaco e usa registros de 52 bytes, alcançando todos os offsets esperados e o fim exato do arquivo. A semântica desses bits continua desconhecida.

Âncoras de código obtidas por referências a strings no MAIN_G:

- AnalysisScript.cpp: candidatos 0x80042A54 e 0x80042D48.
- SceneDraw.cpp: candidato 0x8004CA5C.
- BufferTrackEnd(): bad state: candidato 0x8004FD8C.
- CdSys.cpp: referências na região 0x80023D3C–0x80024130.

Os nomes indicam vizinhanças de código; os limites das funções ainda são hipóteses do analisador. Ele encontrou 1.355 candidatos a função no MAIN_G e deixou 278 transferências indiretas sem alvos provados. São números de descoberta estática, não porcentagem de port concluído.

## Música original e entrada somente MP3

O CUE contém uma faixa de dados e sete faixas de áudio. É necessário preservar o mapa completo, números de faixa e pregaps. Montar apenas o Track 1 perderia informações necessárias à reprodução original.

O código do runtime PSXRecomp inspecionado já implementa CD-DA, consultas de faixa e relatórios de posição/pico. É uma base para teste, sem comprovação de compatibilidade com este jogo. Revisão e referências locais estão em static-europe.json.

Proposta, ainda não implementada:

1. Conservar o disco original durante boot e modo original.
2. Na troca para música externa, disponibilizar um disco de áudio virtual alimentado pelo MP3 selecionado.
3. Decodificar para PCM estéreo de 16 bits a 44.100 Hz. Se o código solicitar setores, oferecer 2.352 bytes por setor: 588 frames estéreo, 75 setores por segundo.
4. Entregar reprodução, análise, duração e posição na mesma linha do tempo. Preservar a leitura que o jogo realmente utiliza, inclusive relatórios ou buffers quando necessários.
5. Tratar atraso/padding do encoder MP3; evitar normalização automática antes de compreender a participação da amplitude.

Ainda precisamos descobrir se o algoritmo observa PCM do disco, relatórios do controlador, buffers SPU, metadados ou uma combinação. A interface de arquivos aceitará somente MP3, como solicitado.

## Caminho para a prova de conceito

Recomendação: testar o [PSXRecomp](https://github.com/RetroPortingToolKit/psxrecomp), usando o Open Ribbon como referência de símbolos e formatos. O framework possui runtime e suporte a overlays; seu analisador já funciona neste computador. Há fallback de interpretação no framework: um build gerado não demonstra, por si só, que todo o código está recompilado.

Próximos critérios de sucesso, ainda pendentes:

1. Gerar C para os quatro módulos, revisar as transições e compilar a aplicação com runtime e CUE completo.
2. Demonstrar boot → título → menu → tutorial/primeira música original, preservando inicialmente a temporização PAL.
3. Comparar gráficos, controles e áudio com uma execução de referência.
4. Rastrear comandos de CD, buffers de áudio e candidatos de AnalysisScript durante música externa conhecida.
5. Integrar MP3 e comparar a mesma música via CD e arquivo, controlando metadados de faixa.

Os arquivos originais foram somente lidos. Executáveis, recursos e disassemblies ficam em .local, ignorado pelo Git. Os relatórios versionáveis contêm metadados e resultados da investigação.
