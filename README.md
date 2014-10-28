# EM-Algorithm in C++

1. Abstract
2. Usage
    1. Command Line Options
    2. Verbose levels
3. Class descriptions
    1. ProbabilisticContextFreeGrammar
    2. Signature
    3. PCFGRule
    4. InsideOutsideCalculator
    5. InsideOutsideCache
    6. EMTrainer
4. Optimizaton & Benchmarks
5. Current issues

## Abstract
This program provides a basic implementation of the inside-outside algorithm, an expectation-maximization algorithm for probabilistic contextfree grammars. It learns the probability distribution for the production rules in an unsupervised way from a text corpus.

## Usage
### Command line options

  --help                  Print help messages
  -g [ --grammar ] arg    Path to a PCFG.
  -c [ --corpus ] arg     Path to the training set with sentences seperated by 
                          newlines.
  -s [ --save ] arg       Path to save the altered grammar
  -o [ --out ]            Output the grammar after the training.
  -i [ --iterations ] arg Amount of training circles to perform. (Default: 3)
  -t [ --threshold ] arg  The changes after the final iteration must be less 
                          equal to this value. Do not combine with  -i.
  -v [ --vlevel ] arg     Define the verbose level (0-10). E.g.: --v=2

**Required: --grammar, -g**
The path to the grammar file, that should be optimized.
The first line of the file contains only the startsymbol of the grammar, while all other lines contain one rule: A lefthandside symbol, an arrow *->* and a righthandside followed by a probability in brackets. Note that the probability is of course optional. If a probability value is given, it is used as the starting value for the training algorithm. The symbols on the righthandside are seperated by blanks. It is not needed to escape special characters as all symbols between two blanks are seen as one symbol.

*Example:*
grammar.pcfg:
S
S -> NP VP [1.0]
NP -> Maria [1.0]
VP -> schläft [1.0]

**Required: --corpus, -c**
The path to the corpus file. It must store one sentence per line and the sentences must be tokenized in a previous step so that all terminal symbols are sperated by blanks.

**--save**
Path to a file to write the newly created PCFG into. Note that rules of the original PCFG that have the probability '0' will not be written into the new file.

**--out**
Print the newly created PCFG to the terminal after the training has been performed.

**--iterations**
*Note: Not to be combined with --threshold*
Specifiy the number of iterations of the algorithm. The default value is 3.

**--threshold**
*Note: Not to be combined with --iterations*
Define a threshold t

**--v=**
Specify the verbose level for detailed information about the programm. See [Verbose Levels] for mor details

### Verbose Levels
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
 
