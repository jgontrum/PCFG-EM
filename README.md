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
Depending on the input sentence and the size of the grammar, choosing a high verbose level can result in hundrets of thousands of messages. To gain usefull information about the most inner processes, choose a very small grammar and a short sentence.)

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


## Class Description
### ProbabilisticContextFreeGrammar
This class manages the PCFG that is represented by a set of rules and a startsymbol. To initialize the class, you have to give the constructor an istream object to a grammar (e.g. a read in file). It then reads the stream line by line and creates PCFGRule objects. The first line is reserved for the starting symbol.
The rules within this grammar can be accessed either by a given lefthandside symbol or by a symbol, that is the first / second nonterminal on the righthandside of a rule (this is very usefull for the inside-outside algorithm).
Internally, the rules are stored in a sorted vector. Asserting that the grammar cannot be changed after the creation process, we then can define intervals for each lefthandside symbol that are values in a map with lhs symbols as values. 
To avoid duplicating the rules for the inside-outside access approach, we crate a vector of constant pointers to rules for each case (symbol is the first / second symbol on the righthandside of a rule).
Another usefull feature of this grammar is the ability to remove all rules with a probability of zero. To do this, it sorts the rules in the index by their probability score and deletes all rules with a zero probability in one step. After this, a reconstruction of the datastructures needed for the access functions is required. Eventhough this sound very inefficent at first, it actually increases the speed of further training iterations (see 'Optimization & Benchmark' for more details).

### Signature
To speed up the comparision of symbols, a signature is used to translate their string representaions to integers and vice versa.
The implementations is quite simple: A hashmap maps strings (or other objects since the class is implemented as template) to their numeric equivalent and a vector of strings is organized in a way that the index of each item is its numeric representation.

### PCFGRule
A PCFGRule can only be created from a string that is automatically parsed. To do so, the constructor needs a reference to a signature as well, so it can translate the string of the symbols to numeric values. If the parsing fails, the whole object becomes invalid.
Though lefthandside and righthandside cannot be changed, a PCFGRule object has a method to alter its probability value. 
Notable are a subclass 'Hasher' that is needed to compute the hashcode for integer representations of the symbols and the struct 'ProabilityComparator' that allow to sort PCFGRule objects only by their probability score.

### InsideOutsideCalculator
Here the algorithm to calculate inside and outside probabilities is implemented (based on 'Foundations of Statistical Natural Language Processing' by  Manning and Schütze). For each new sentence, a new instance of this calculator should be created, beacause it uses a cache to prevent the multiple calculation of the same data. The calculated estimates depend on the terminal symbols (base case of the inside algorithm), so it is not possbile to use same cache for multiple sentences. 
To calculate the inside estiamte of a symbol for a span, call 'calculate_inside' with a reference to the symbol, the index of the beginning of the span and to the end of the span. The outside probability is calculated by 'calculate_putside'. This method as well takes a reference to a symbol and a number of words to the left and to the right. Further details about the algorithms themselves can be found in the comments of the code.

### InsideOutsideCache
Each InsideOutsideCalculator object contains a cache to save the calculated values for the (Symbol, Integer, Integer) triples. This cache simply maps the triples to their score in two seperate hashmaps, one for inside and one for outside values. 
Instead of using a pair of pairs to represent the triple, the cache concatenates the bits of the three varibales to a 64 bit variable that is used as key in the maps. This approach makes the assumption, that sum of the bits of the variable does not exceed 64 bit. By choosing a 32 bit integer value for the symbol (more than enough space to store millions of symbols) and 8 bit integers for the two other variables, this criteria is matched. The two 8 bit variables store only information about the sentence itself and since sentences longer than 255 tokens should neither exist in a treebank, nor is et virtually possible to parse a sentence of this lemngth in adequate time, it should not be a problem. 
Here is an example for the bit concatenation:

Let's assume we have the variables s, x and y where s is a 32 bit variable and x and y can store 8 bits.
We set s = 123456, x = 3, y = 12.
Now the bit pattern for the variables are now as following:
s : 1110001001000000
x : 0011
y : 1100

We copy the bits of s in the 64 bit buffer b:
b : 0000000000000000000000000000000000000000000000001110001001000000
Now we concatenate it with x:
b : 0000000000000000000000000000000000000000000011100010010000000011
And y:
b : 0000000000000000000000000000000000000000111000100100000000111100

This way, the three variables have been combined to one unique key without hashing (althoug it will be cashed again by the map). In the 'Optimization & Benchmark' section I talk about the performace of this procedure.

### EMTrainer
This class performs the actual training of the PCFG. It is initialized with a reference to an istram to a training corpus, wich is read in line by line, tokenized and translated to symbols of the signature of the PCFG. If a sentence contains an unknown symbol, the sentence will be ignored becaue it cannot get estimates > 0.
The trainer can be run in two different modes: Either with a given number of iterations or a threshold. In the first mode, the training algorithm is simply called as often as specified. The other mode perfomes the training, until the changes in the PCFG are equal to or below the threshold. The changes are calculated by summing up the differences in the probabilities of all rules in the grammar.
This value is the output if the private *train()* method wich performes the training by calculating the inside probability of each sentence in the corpus and estimating the symbol expectation for all nonterminals for each sentence. This is rather straight forward implementation of the algorithm that can be found in 'Foundations of Statistical Natural Language Processing' by Mannoing and Schütze.

## Optimizaton & Benchmarks
After using a profiler to ensure that the program contains neither memory leaks nor extemly slow functions, I found out that the biggest performance bottleneck is the read access of the cache.

### Caching
This cache itself extremely boosts the speed of the calculation. As an example, I am performing a training with only one iteration for a very small sentence (four tokens) and a grammar extracted from section 00 of the Wall Street Journal (7466 rules).
Even in this very small example, there were overall 677579 calls for calculating inside or outside estimates. With the help of a cache, 92% of this calculations have been prevented by a lookup in the cache (622079 read accesses). Of course, this ratio increases with the number of iterations.

To improve the overall performance, there are two possibilities: one is to increase the speed of the cache itself an the other is to prevent redundant calculations in the first place.

### Removing rules with zero probability
Following the second approach first, all redundant rules will be removed from the grammar after the first iteration. The idea is, that if a probability of zero is assigned to a rule after the first training, it means that it has never been used in the given corpus. Since the inside-outside algorithm aims to improve the proability of all rules based on the sentences of a corpus, this zero probability rules will not play a role in further iterations, so they can be removed.
The smaller the corpus, the higher is the percentage of redundant rules. In case of the small four-token sentence mentioned before, there have been 677579 calculations in the first iteration but only 4646 after removing the rules (0.001%).
In cases with more and longer senctences, the speed of the program increased up to 50%.

### Optimizing the cache
Eventhough minimizing the number of accesses to the cache improbes the performance, it is crucial to optimize the cache itself. 
The cache uses a triples of a symbol and two integers as keys. Creating two pair objects for each lookup in the cache can be very time consuming. To get rid of this, the bits of the variables will be concatenated into one 64 bit variable, that then will be used as the key in the hashmap (see InsideOutsideCache for more details). Of course it is still neccecary to compute the hashvalue of the key, but by avoiding the creation if the pair objects, the programm runs about 20% faster.

Using a 32bit variable instead of 64bit one had no measurable effect. 

### Different HashMap implementations
This programm only uses the unordered map from std. Using other implementations like boost decreased the speed by about 30%.


