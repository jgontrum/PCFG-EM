### PCFG-EM (incomplete documentation)
## Verbose Levels
To learn more about the inner processes of this program, you can activate the verbose mode. To do so, choose the option *--v=* followed by an interger between 1 and 10 while 1 output only a few messages and 10 makes the programm print all verbose messages.

**Note:**
Depending on the input sentence and the size of the grammar, choosing a high verbose level can result in hundrets of thousands of messages. To gain usefull information about the most inner processes, choose a very small grammar and a short sentence.

| Level | Messages |
|-------|----------|
| 1     | EMTrainer: Detailed results of the training.
| 2     | EMTrainer: Results per iteration
| 3     | EMTrainer: Current sentence and inside probability of the current sentence
| 4     | EMTrainer: Training corpus read in
|       | PCFG: General cleaning process
| 5     | PCFG: Detailed cleaning process, startsymbol
| 6     | EMTrainer: Result of symbol and rule estimation, line-by-line corpus read in
| 7     | InsideOutsideCalculator: General information about the calculations
| 8     | InsideOutsideCalculator: Detailed information about the calculations
| 9     | PCFGRule: Rule created successfully
|       | Signature: Index of new item in the signature
|       | EMTrainer: Per rule probability update
| 10    | InsideOutsideCache: Result of the bit concatenation of the map keys
 
