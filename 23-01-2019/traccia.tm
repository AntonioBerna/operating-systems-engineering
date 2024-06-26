<TeXmacs|2.1.4>

<style|generic>

<\body>
  <doc-data|<doc-title|Esame di Sistemi Operativi>>

  <section|Traccia>

  Implementare una programma in linguaggio C che riceva in input, tramite
  <cpp|*argv[]>, il nome di un file <math|F> ed <math|n> stringhe
  <math|s<rsub|1>,\<ldots\>,s<rsub|n>> (con <math|n\<geqslant\>1>). Per ogni
  stringa <math|s<rsub|i>> dovr� essere attivato un nuovo thread
  <math|T<rsub|i>>, che funger� da gestore della stringa <math|s<rsub|i>>. Il
  main thread dovr� leggere indefinitamente stringhe dallo standard-input.
  <with|font-series|bold|Ogni nuova stringa letta dovr� essere comunicata a
  tutti i thread> <math|T<rsub|1>,\<ldots\>,T<rsub|n>> tramite un buffer
  condiviso, e ciascun thread <math|T<rsub|i>> dovr� verificare se tale
  stringa sia uguale alla stringa <math|s<rsub|i>> da lui gestita. In caso
  positivo, ogni carattere della stringa immessa dovr� essere sostituito dal
  carattere <cpp|'*'>.\ 

  Dopo che i thread <math|T<rsub|1>,\<ldots\>,T<rsub|n>> hanno analizzato la
  stringa, ed eventualmente questa sia stata modificata, il main thread dovr�
  scrivere tale stringa (modificata o non) su una nuova linea del file
  <math|F>. In altre parole, la sequenza di stringhe provenienti dallo
  standard-input dovr� essere riportata sul file <math|F> in una forma
  <with|font-shape|italic|\Pepurata\Q> \ delle stringhe
  <math|s<rsub|1>,\<ldots\>,s<rsub|n>>, che verranno sostituite da stringhe
  della stessa lunghezza costituite esclusivamente da sequenze del carattere
  <cpp|'*'>.

  Inoltre, qualora gi� esistente, il file <math|F> dovr� essere troncato (o
  rigenerato) all'atto del lancio dell'applicazione. Infine, l'applicazione
  dovr� gestire il segnale <cpp|SIGINT> in modo tale che quando il processo
  venga colpito esso dovr� riversare su standard-output il contenuto corrente
  del file <math|F>.

  <section|Note sulla soluzione proposta>

  Due possibili soluzioni sono riportate nel file <cpp|prog.c>, infatti sono
  presenti due implementazioni per la gestione dei segnali richiesti. La
  prima utilizza la funzione <cpp|void (*signal(int, void (*)(int)))(int)>,
  mentre la seconda utilizza la struttura <cpp|struct sigaction> insieme alla
  system call <cpp|int sigaction(int, struct sigaction *, struct sigaction
  *)>. In particolare sar� sufficiente cambiare il valore della seguente
  macro:

  <\numbered>
    <\cpp-code>
      #if 0 // change this value

      \ \ \ \ // use signal()

      #else

      \ \ \ \ // use struct sigaction and sigaction()

      #endif
    </cpp-code>
  </numbered>

  Mi sono preso la libert� di gestire anche il segnale <cpp|SIGQUIT>
  (l'equivalente di <shell|ctrl+\\> indispensabile per la terminazione del
  programma nel momento in cui, gestendo il segnale <cpp|SIGINT>, il
  programma non termina), il quale verr� gestito da una routine per la
  liberazione dei semafori System V allocati. Ricordo che questo check si pu�
  fare durante lo sviluppo del codice utilizzando il comando <shell|ipcs -s>.

  Per generare il file eseguibile, � possibile utilizzare il comando
  <shell|./build.sh> o il comando <shell|./build.sh debug> in base al livello
  di interazione uomo-macchina che si desidera. Chiaramente le funzionalit�
  in modalit� Debug aiutano a capire meglio cosa sta succedendo. Per
  utilizzare l'eseguibile appena generato � possibile utilizzare il comando
  <shell|./build/prog> che mostrer� a schermo il seguente output:

  <\numbered>
    <\shell-code>
      $ ./build/prog

      Usage: ./build/prog \<less\>filename\<gtr\> \<less\>str-1\<gtr\> ...
      \<less\>str-N\<gtr\>
    </shell-code>
  </numbered>

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

      <vspace*|1fn><with|font-series|<quote|bold>|math-font-series|<quote|bold>|2<space|2spc>Note
      sulla soluzione proposta> <datoms|<macro|x|<repeat|<arg|x>|<with|font-series|medium|<with|font-size|1|<space|0.2fn>.<space|0.2fn>>>>>|<htab|5mm>>
      <no-break><pageref|auto-2><vspace|0.5fn>
    </associate>
  </collection>
</auxiliary>