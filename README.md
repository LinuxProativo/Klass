<p align="center">
  <img src="klass.png" width="280"/>
</p>

<h1 align="center"><strong>Klass - Gerenciador de Pacotes para o Slackware</strong></h1> 

<p align="center">
    <img src="https://img.shields.io/badge/Platform-Slackware-004482?style=flat-square&logo=slackware&logoColor=white"/>
    <img src="https://img.shields.io/badge/Build-CMake%203.27-064F8C?style=flat-square&logo=cmake&logoColor=white"/>
    <img src="https://img.shields.io/badge/Language-C%2B%2B_26-00599C?style=flat-square&logo=cplusplus&logoColor=white"/>
    <img src="https://img.shields.io/github/actions/workflow/status/LinuxProativo/klass/rust.yml?label=Test&style=flat-square&logo=github"/>
    <img src="https://img.shields.io/github/languages/code-size/LinuxProativo/klass?style=flat-square&logo=paperlessngx&label=Code%20Size"/>
    <img src="https://img.shields.io/github/repo-size/LinuxProativo/klass?style=flat-square&logo=paperlessngx&label=Repo%20Size"/>
    <img src="https://img.shields.io/github/license/LinuxProativo/klass?color=673ab7&label=License&style=flat-square&logo=opensourcehardware&logoColor=white"/>
</p> 


## 🔍 Visão Geral

> VERSÃO BETA 3 - AVISOS
>
> - AINDA não há verificações relacionadas ao uso de NVIDIA.
> - Pode precisar de ajustes ao gerar um novo initrd.
> - Ainda não há rotina pós-instalação para mapear arquivos.new.
> - Atualização do grub e lilo pode precisar de ajustes.
> - Reinstalação do módulo do VirtualBox Experimental.
>
> Ajustes já mapeados para as próximas versões estáveis:
> - detecção de arquivos .new
> - tentativa de autoreinstalação de modulos dkms.
> - IMPLEMENTAR barra de status.
> - opção de usar ou não o repo testing e extra.
> - Rollback em caso de necessidade.
> - download paralelo opcional.
> - suporte a checksum mais seguros se disponíveis.
> - Opção para setar assinatura é opcional ou obrigatória para mirrors específicas.
> - regras avançadas de controle manual.
> - Menu para atualizar a base de dados sem abrir a interface.
> - opção de proceder com a atualização do sistema. após o update.

O **Klass** (Slac**K**ware c**LASS**ification Package Manager) é um gerenciador de
pacote gráfico desenvolvido em `C++ moderno`, usando como interface gráfica o `Qt`.
Ele foi desenvolvido com o objetivo de facilitar o gerenciamento de pacotes no
**Slackware**, reduzir a possibilidade de quebrar o sistema e facilitar a manutenção
de um sistema minimalista, melhorando o controle do usuário sobre o sistema e pacotes
instalados.

No `Slackware Current`, ele ajuda o usuário a não esquecer de instalar novos pacotes
adicionados em uma categoria específica, evitando possíveis problemas com falta
de dependência. Já no `Slackware Stable`, ele ajuda a visualizar pacotes disponíveis
para atualização. E para o usuário que deseja usar a ramificação testing, é possível
gerenciá-lo com muita facilidade, sem correr o risco de quebrar o sistema.

O gerenciador tem suporte de exibição por categoria e o controle de atualização foi
feito levando em conta uma lista de prioridades e bloqueio, e também as últimas
atualizações por repositórios, e não as últimas atualizações de pacotes no geral,
o que evita possíveis riscos de quebra do sistema e também reduz a probabilidade
de quebra de pacotes de terceiros instalados no sistema porque misturou os pacotes
de diferentes repositórios. Você também pode gerenciar as atualizações de
repositórios individualmente se você quiser.

## 🪶 Filosofia

Como sabemos, a principal filosofia original do Slackware é o `KISS`
(**K**eep **I**t **S**imple, **S**tupid), que na prática se traduz no famoso estilo
`DIY` (**D**o **I**t **Y**ourself), ou seja, **faça você mesmo**. A distribuição
geralmente evita interfaces gráficas de configuração e ferramentas de automatização
complexas, exigindo que o usuário interaja diretamente com arquivos de texto e linhas
de comando.

A ideia de um gerenciador de pacotes gráficos parece contrário a essa filosofia. No
entanto, essa ferramenta não tenta fazer nada complexo e sem intervenção prévia do
usuário, respeitando a filosofia original do Slackware. A única automação é uma simples
verificação de atualização da base de dados dos repositórios, para que o usuário decida
se vai atualizar a base de dados e o sistema ou não, o que é algo simples.

A interface mantém uma experiência original de terminal no processo de atualização da
base de dados, como se você estivesse fazendo isso via terminal mesmo. O gerenciamento
geral dos pacotes (instalação, atualização, etc) segue a mesma lógica.

## ✨ Funcionalidades

- **🪄 Atualização do DataBase**  
  Verificação automática de atualizações da base de dados de todos os repositórios
  configurados. O usuário decide se vai atualizar a base de dados e o sistema. Não
  terá nenhuma opção para tentar automatizar esse processo, pois isso exigiria um
  helper privilegiado em background desde a inicialização do sistema, além da
  dificuldade de depurar depois, qualquer possibilidade de quebra do sistema por 
  conta de uma regra mau definida, por exemplo.

- **🔄 Atualização do Sistema**  
  A atualização do sistema vai funcionar levando em conta as atualizações de
  pacotes por repositório e não por versão mais alta disponível entre todos os
  repositórios configurados. Em relação ao repositório oficial, `testing`
  **(se configurado)** prioriza `patches`, que prioriza os pacotes oficiais
  originais, já que no Slackware Stable, as atualizações ficam em `patches`.
  Repositórios de terceiros não se misturam, ao instalar/atualizar os pacotes,
  as dependências são buscadas apenas naquele repositório em que o pacote se encontra,
  pois assim é mais garantido que vai funcionar. Qualquer decisão diferente dessa,
  cabe ao usuário decidir manualmente e se preciso, criar as devidas regras.

- **🌍 Configuração da Mirror Oficial**  
  O projeto se aproveita da lista conveniente do  `slackpkg` em `/etc/slackpkg/mirrors`
  para montar uma lista de repositórios disponíveis e já aproveita e faz um teste de
  latência que mostra as melhores opções de espelhos com o menor tempo de resposta,
  facilitando a escolha da mirror mais adequada. Além disso, uma mirror oficial
  diferente pode ser adicionada manualmente.

- **📂 Suporte a Repositórios de Terceiros**  
  Foram testados uma série de repositórios de terceiros que podem ser selecionados
  através de uma lista que vai mostrar as opções compatíveis com o seu sistema.
  Não achei necessário implementar testes de latência nesse caso. Também será possível
  adicionar repositórios adicionais manualmente.

- **🔝 Gerenciamento de Prioridades e Excessões**  
  Função importante e útil para adicionar algumas regras de excessões que podem ser
  por pacote ou por categoria, também pode priorizar pacotes por repositório que é
  útil para priorizar um pacote de terceiro acima de um pacote do repositório oficial
  do Slackware, por exemplo, ffmpeg.

- **🚀 Configuração de Pós-Instalação**  
  O programa conta com algumas opções de pós-instalações que podem ser configurados
  para ser executado após a instalação dos pacotes ou atualização do sistema. O
  usuário poderá acessar as configurações do programa e escolher quais opções de
  pós-instalação faz sentido para ele.

- **📦 Gerenciamento de Pacotes Disponíveis até o Momento**

  - **Instalação:** Suporte a instalação com busca de dependências adicionais, usa
    `upgradepkg --install-new` para instalar.

  - **Atualização:** Suporte a atualização com busca de possíveis novas dependências,
    usa `upgradepkg --install-new` para atualizar.

  - **Reinstalação:** Suporte e reinstalação de pacotes se disponível em cache ou se
    disponível para download, usa `upgradepkg --install-new --reinstall` para
    reinstalar.

  - **Desinstalação:** Suporte básico de pacotes, não tenta caçar pacotes desnecessários.
    Remover pacotes desnecessários é por conta do usuário. Usa `removepkg` para remover.

## 🧩 Dependências

O Klass pode ser considerado um front-end para o `pkgtools`. Ele é feito para funcionar
no `Slackware` sem dependências adicionais. Mas para um sistema minimalista, é necessário
garantir alguns requisitos:

- **pkgtools:** Gerenciador de pacotes do Slackware. (Nem precisava pontuar).
- **slackpkg:** É sério, por causa do acesso ao `/etc/slackpkg/mirrors`. (Duvido que
alguém seria doido de mexer nesses pacotes, mas vale o aviso).
- **libnotify:** Suporte a notificação.
- **gpgme:** Verificação de chave GPG e assinatura.
- **polkit:** Pacote do binário pkexec.
- **qt6:** Interface gráfica.

> **OBSERVAÇÃO:**
>
> - Para o `Slackware 15.0 Stable` é necessário a instalação do `Qt6`.
> - Pode haver mais pacotes do `polkit` que pode ser necessário.
> - Pode haver novos requisitos no futuro, a medida que o projeto evoluir.

## 📥 Compilação e Instalação

Se você preferir compilar o pacote, vai precisar do kit de desenvolvimento `d`
instalado no Slackware, o que já é o padrão em uma instalação completa.

### ⚙️ Build Tradicional

Segue o método convencional e totalmente manual.

```sh
$ git clone https://github.com/LinuxProativo/klass.git
$ cd klass

### CMake Padrão

$ mkdir build && cd build
$ cmake -DCMAKE_BUILD_TYPE=Release ..
$ make -j$(($(nproc) + 1)) 
$ sudo make install

### CMake + Ninja

$ mkdir build && cd build
$ cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
$ ninja
$ sudo ninja install

```

### 📦 Com SlackBuild

O melhor mesmo é gerar o pacote.

```sh
$ git clone https://github.com/LinuxProativo/klass.git
$ cd klass/slackware
$ chmox +x ./klass.SlackBuild
$ sudo ./klass.SlackBuild 
```

Essa versão do Slackbuild já faz a instalação e atualização do pacote no sistema.

## ⭐️ Experiência de usuário

A interface, a arquitetura e o comportamento do programa é pensado em experiência de
usuário e redução de curva de aprendizagem. Dentre os principais:

- Interface simplificada.
- Ícone na área de notificação e opção de inicialização com o sistema.
- Digita a senha de superusuário apenas uma vez quando é soliciado, isso inicia um
  helper privilegiado que só tem o seu privilégio revogado 30s aṕos fechar o programa
  para a área de notificação. Assim, não fica precisando colocar senha um monte de vez. 
- As informações de pacotes nas tabs de `Informações` e `Arquivos` são copiáveis.
- **EM BREVE:** Pacotes baixados manualmente pode ser instalados via interface usando klass.
- **EM BREVE:** Pacotes deb e rpm poderão ser convertidos e instalados facilmente.

## ⚙️ Comportamento e Configurações

O gerenciador de pacotes Klass usa o diretório /var/lib/klass para baixar a base de dados
e fazer o download dos pacotes para a instalação:

- `/var/lib/klass/database`  
  Onde são baixados a dase de dados de pacotes dos repositórios.

- `/var/lib/klass/packages`  
  Onde são baixados os pacotes para a instalação ou atualização.

- `/etc/klass/klass.conf`  
  Onde ficam as configurações do gerenciador de pacotes. 


## 🛠️ Opções de Pós-Instalações Disponíveis

Ferremantas oficiais do Slackware não costumam automatizar operações de
pós-configurações. Mas o Klass possui configurações de pós-instalação que podem
ser ativadas nas configurações do programa. Nesse caso, o usuário decide quais
opções ativar para executar após as opreações de pacotes. Dentre elas:

1. **Atualizar cache de bibliotecas compartilhadas (`ldconfig`)**
   - **Comando:** `/sbin/ldconfig`  
     Atualiza as ligações simbólicas e o cache de memória do sistema para bibliotecas
     dinâmicas (`.so`) recém-instaladas.

2. **Atualizar banco de dados MIME (`update-mime-database`)**
   - **Comando:** `/usr/bin/update-mime-database /usr/share/mime`  
     Registra e atualiza os tipos de arquivo e associações do sistema para os
     aplicativos.

3. **Atualizar banco de páginas de manual (`mandb`)**
   - **Comando:** `/usr/bin/mandb -q`
     Reconstrói o índice do banco de dados das páginas de manual (`man`).

4. **Atualizar banco de dados de arquivos Desktop (`update-desktop-database`)**
   - **Comando:** `/usr/bin/update-desktop-database -q`  
     Atualiza o cache dos arquivos `.desktop`, garantindo que novos programas
     apareçam corretamente no menu do ambiente gráfico.

5. **Atualizar cache de ícones do GTK (`gtk-update-icon-cache`)**
   - **Comando:** `/usr/bin/gtk-update-icon-cache -q -t -f /usr/share/icons/hicolor`  
     Regenera o cache de temas de ícones para acelerar a renderização visual dos
     aplicativos.

6. **Compilar esquemas do GLib (`glib-compile-schemas`)**
   - **Comando:** `/usr/bin/glib-compile-schemas /usr/share/glib-2.0/schemas`  
     Compila os arquivos de esquemas XML do GSettings/GLib para o formato binário
     usado pelos programas.

7. **Atualizar carregador de inicialização GRUB (`grub-mkconfig`)**
   - **Comando:** `/usr/sbin/grub-mkconfig -o /boot/grub/grub.cfg`  
     Regenera o arquivo de menu do GRUB útil após instalações ou atualizações de
     kernel.

8. **Atualizar carregador de inicialização LILO (`lilo`)**
   - **Comando:** `/sbin/lilo`  
     Grava as novas referências de boot na MBR do disco conforme o `/etc/lilo.conf`.

9. **Gerar initrd para kernel genérico (`mkinitrd`)**
   - **Comando:** `/usr/share/mkinitrd/mkinitrd_command_generator.sh -r` ou `/sbin/mkinitrd -F`  
     Gera automaticamente o disco inicial em RAM (`initrd`) necessário para dar boot
     com o kernel `generic`.

10. **Recompilar módulos de kernel do VirtualBox (`rc.vboxdrv setup`)**
    - **Comando:** `/etc/rc.d/rc.vboxdrv setup` ou `/sbin/rc.vboxdrv setup`  
      Recompila e reinstala os módulos de driver do VirtualBox contra os cabeçalhos
      do kernel atual.

## 🛡️ Segurança do Helper

Geralmente, o **Helper** é um serviço executado com privilégios de **root** responsável
por realizar operações privilegiadas que podem ser solicitadas por algum programa sendo
executado como usuário comum.

Como a comunicação no **Klass** ocorre através de um **QLocalServer** configurado com
acesso global (`WorldAccessOption`), foram implementadas diversas camadas de validação
para garantir que o Helper seja utilizado apenas pelo próprio programa. Dentre eles:

- **Autenticação do Cliente Utilizando `SO_PEERCRED`**  
  Ao receber uma nova conexão, o Helper obtém as credenciais reais do processo remoto
  utilizando `SO_PEERCRED`, fornecido pelo kernel Linux. Com essas informações,
  é possivel verificar:
    * PID do processo cliente;
    * UID (usuário);
    * GID (grupo).

  Esses dados são obtidos diretamente pelo kernel e não podem ser forjados por um
  cliente modificado através do protocolo de comunicação. Assim, já previne que certos
  indivíduos usem um cliente modificado para executar operações no helper conforme
  veremos a seguir.

- **Validação do Executável Através de `/proc/<pid>/exe`**  
  Além da identidade do processo, validamos quem iniciou a conexão com o Helper.
  Para isso, é obtido o caminho real do programa através de `/proc/<pid>/exe`.
  O caminho é resolvido utilizando `canonicalFilePath()` e comparado com o caminho do
  próprio binário do Helper. Caso o processo conectado não corresponda exatamente ao
  executável esperado, a conexão é imediatamente rejeitada. Quero ver passar por essa. 

- **Rejeição de Clientes Privilegiados**  
  O Helper também verifica o UID do processo conectado. Conexões de processos executados
  como **root** são rejeitadas, permitindo apenas clientes executados como usuário comum.
  Estou assumindo que ninguém vai usar Slackware como `root` em condições normais.
  > **OBS:** Se isso for um inconveniênte para alguém, talvez eu crio uma config para isso.

- **Confirmação dos privilégios do Helper**  
  Antes de aceitar qualquer operação privilegiada, o Helper verifica se realmente está
  sendo executado como root (`geteuid() == 0`). Caso contrário, nenhuma requisição é
  aceita. Isso é mais uma prevenção contra possíveis falhas.

- **Protocolo Fechado**  
  Aqui sim resolve para segurança real. O protocolo de comunicação aceita apenas um
  conjunto fixo de comandos previamente implementados, como instalação,
  remoção de pacotes, atualização da base de dados e gerenciamento de repositórios.
  Mensagens desconhecidas são descartadas e registradas em log. Não existe interpretação
  dinâmica de comandos enviados pelo cliente.

- **Não Executa Comandos Arbitrários**  
  O Helper **não executa linhas de comando recebidas do cliente**. A interface apenas
  solicita operações previamente definidas, e o Helper decide internamente qual rotina
  será executada. Esse modelo impede que um processo cliente malicioso utilize o Helper
  para executar comandos arbitrários com privilégios de root, reduzindo
  significativamente a superfície para escalonamento de privilégios, que poderia ser
  explorado por certos indivíduos.

- **Cliente Único**  
  O Helper aceita apenas **uma conexão ativa por vez**. Qualquer tentativa de
  estabelecer uma segunda conexão simultânea é imediatamente rejeitada. Isso simplifica
  o controle de estado interno e evita concorrência entre clientes. E por fim,
  como o klass se conecta imediatamente ao abrir o Helper, nunca que um processo cliente
  modificado teria tempo de ser executado e de pular na frente do programa para se
  conectar ao Helper, a menos que esteja em segundo plano esperando a oportunidade de
  tentar `(o chamado Race Condition)` e ser rejeitado pelas demais proteções. 

- **Isolamento via Namespace Abstrato (Abstract Namespace)**  
  Para eliminar riscos associados a arquivos residuais no sistema de arquivos, o
  servidor de comunicação local (`QLocalServer`) foi configurado utilizando a
  opção `AbstractNamespaceOption` junto com `WorldAccessOption`. Isso traz duas
  grandes vantagens de segurança e resiliência:

    * **Inexistência de Arquivo Físico**  
      Com o `AbstractNamespaceOption`, o socket não cria um arquivo especial em
      diretórios públicos (como `/tmp`). Com isso, elimina-se completamente qualquer
      vetor de ataque baseado em manipulação de links simbólicos (Symlink Attacks),
      Race Conditions no sistema de arquivos ou sequestro de caminhos de arquivos.

    * **Limpeza Garantida pelo Kernel**  
      O socket fica vinculado diretamente ao ecossistema de rede do kernel Linux.
      Se o Helper ou o sistema sofrerem uma queda inesperada, o socket é destruído
      instantaneamente pelo sistema operacional. Isso evita o problema de sockets
      **"órfãos"** no sistema, que poderiam bloquear reinicializações ou ser
      inspecionados por invasores.

- **Validação das Requisições**  
  Antes de executar as operações, o Helper realiza validações básicas dos parâmetros
  recebidos, ignorando requisições malformadas ou incompletas.

- **Encerramento Automático por Inatividade**  
  Um temporizador de inatividade encerra automaticamente o Helper quando ele deixa de
  ser utilizado, reduzindo o tempo de exposição do processo privilegiado em execução.

## 🚀 Futuras Funcionalidades

Segue as implementações para o futuro:
  - Melhorias na interface.
  - Rollback de pacotes.
  - Suporte a gerenciamento de Slackbuilds nativo ou plugin sbopkg.
  - Conversão e instalação de pacotes RPM.
  - Conversão e instalação de pacotes DEB.
  - Suporte a configuração de plugins adicionais:
    - cpak.
    - dbin.
    - soar.
  - Verificação de dependencias estimadas.
  - Integrador AppImage.
  - Plugin de configuração e instalação de drivers nvidia.
  - Remoção de pacotes obsoletos oficiais. 

A remoção de pacotes obsoletos vai ignorar por padrão pacotes de terceiros e só
vai agir em pacotes sem tag e tags tipo _slack*. Para quem criar os próprios pacotes,
adicionar suas próprias tags ajudam a evitar remoções acidentais.

> **INTEGRAÇÃO FLATPAK:**  
> Só SSSSEEEEE eu tiver muita vontade, SSSSEEEEE eu achar que vai ser útil e vai valer
> a pena e SSSSEEEEE alguém insistir MMMMMUUUUUITO, aí talvez eu invente um plugin
> adicional.
>
> **INTEGRAÇÃO NIX:**  
> Sei que tem gente que usa no Slackware e não vive sem, mesmos termos acima.
>
> **INTEGRAÇÃO SNAP:**  
> Eu já vi gente (pelo menos um) querendo isso no Slackware. Não vai dá não.

## Contrubuição

Contribuições são bem-vindas.

## 📬 Contato & Suporte

* 📧 **Email:** [m10ferrari1200@gmail.com](mailto:m10ferrari1200@gmail.com)
* 📧 **Email:** [contatolinuxdicaspro@gmail.com](mailto:contatolinuxdicaspro@gmail.com)
