<TeXmacs|2.1>

<style|generic>

<\body>
  <doc-data|<doc-title|Esame di Sistemi Operativi>>

  <section|Traccia>

  Implementare una programma in linguaggio C che riceva in input, tramite
  <cpp|*argv[]>, il nome di un file <math|F> ed <math|n> stringhe
  <math|s<rsub|1>,\<ldots\>,s<rsub|n>> (con <math|n\<geqslant\>1>). Per ogni
  stringa <math|s<rsub|i>> dovrà essere attivato un nuovo thread
  <math|T<rsub|i>>, che fungerà da gestore della stringa <math|s<rsub|i>>. Il
  main thread dovrà leggere indefinitamente stringhe dallo standard-input.
  <with|font-series|bold|Ogni nuova stringa letta dovrà essere comunicata a
  tutti i thread> <math|T<rsub|1>,\<ldots\>,T<rsub|n>> tramite un buffer
  condiviso, e ciascun thread <math|T<rsub|i>> dovrà verificare se tale
  stringa sia uguale alla stringa <math|s<rsub|i>> da lui gestita. In caso
  positivo, ogni carattere della stringa immessa dovrà essere sostituito dal
  carattere <cpp|'*'>.\ 

  Dopo che i thread <math|T<rsub|1>,\<ldots\>,T<rsub|n>> hanno analizzato la
  stringa, ed eventualmente questa sia stata modificata, il main thread dovrà
  scrivere tale stringa (modificata o non) su una nuova linea del file
  <math|F>. In altre parole, la sequenza di stringhe provenienti dallo
  standard-input dovrà essere riportata sul file <math|F> in una forma
  <with|font-shape|italic|\Pepurata\Q> \ delle stringhe
  <math|s<rsub|1>,\<ldots\>,s<rsub|n>>, che verranno sostituite da stringhe
  della stessa lunghezza costituite esclusivamente da sequenze del carattere
  <cpp|'*'>.

  Inoltre, qualora già esistente, il file <math|F> dovrà essere troncato (o
  rigenerato) all'atto del lancio dell'applicazione. Infine, l'applicazione
  dovrà gestire il segnale <cpp|SIGINT> in modo tale che quando il processo
  venga colpito esso dovrà riversare su standard-output il contenuto corrente
  del file <math|F>.

  <section|Soluzione>

  Una possibile soluzione è riportata nel file <cpp|prog.c> e, per generare
  il file eseguibile, è possibile utilizzare il comando <shell|./build.sh> o
  il comando <shell|./build.sh debug> in base al livello di interazione
  uomo-macchina che si desidera. Chiaramente le funzionalità in modalità
  Debug aiutano a capire meglio cosa sta succedendo. Per utilizzare
  l'eseguibile appena generato è possibile utilizzare il comando
  <shell|./build/prog> che mostrerà a schermo il seguente output:

  <\shell-code>
    $ ./build/prog

    Usage: ./build/prog \<less\>filename\<gtr\> \<less\>str-1\<gtr\> ...
    \<less\>str-N\<gtr\>
  </shell-code>

  In presenza di problematiche provare ad eseguire il comando <shell|chmod +x
  build.sh> e nuovamente il comando <shell|./build.sh> o il comando
  <shell|./build.sh debug>.
</body>

<\initial>
  <\collection>
    <associate|page-medium|paper>
  </collection>
</initial>

<\references>
  <\collection>
    <associate|auto-1|<tuple|1|1>>
    <associate|auto-2|<tuple|2|1>>
  </collection>
</references>

<\auxiliary>
  <\collection>
    <\associate|toc>
      <vspace*|1fn><with|font-series|<quote|bold>|math-font-series|<quote|bold>|1<space|2spc>Traccia>
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-1><vspace|0.5fn>

      <vspace*|1fn><with|font-series|<quote|bold>|math-font-series|<quote|bold>|2<space|2spc>Soluzione>
      <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-2><vspace|0.5fn>
    </associate>
  </collection>
</auxiliary>