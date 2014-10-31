
# Values for Linux.
# For OS X, please remove the -lboost_program_options option and specify the path 
# to the boost library yourself.
CPPCOMPILER         = clang++
COMPILER_FLAGS      = -O2 -std=c++11 -lboost_program_options 
DELETE              = rm -f
DELETE_RECURSIVE    = rm -f -r
EXECUTABLE          = -o bin/pcfgem
DOC_GENERATOR       = doxygen doc/doxygen
INCLUDE_PATH        = include/
SRC_PATH            = src/
DOC_PATH            = doc/


# Option 1: Generate the program and the documentation
all : build documentation

# - Creates the executable
build : bin/ bin/pcfgem  

bin/ : 
	mkdir bin/

bin/pcfgem : $(SRC_PATH)main.cpp $(HEADERFILES)
	$(CPPCOMPILER) $(COMPILER_FLAGS) $(SRC_PATH)main.cpp $(EXECUTABLE)

# - Headerfiles
$(HEADERFILES) : $(HEADER_TRAINER) $(EASYLOGGING) $(HEADER_GRAMMAR) $(HEADER_INSIDEOUTSIDE)

# - Headerfiles related to the parser
$(HEADER_TRAINER) : $(INCLUDE_PATH)EMTrainer.hpp

# - Headerfiles related to inside outside calc
$(HEADER_INSIDEOUTSIDE) : $(INCLUDE_PATH)InsideOutsideCache.hpp $(INCLUDE_PATH)InsideOutsideCalculator.hpp

# - Headerfiles related to the grammar representation
$(HEADER_GRAMMAR) : $(INCLUDE_PATH)ProbabilisticContextFreeGrammar.hpp $(INCLUDE_PATH)PCFGRule.hpp $(INCLUDE_PATH)Signature.hpp 

# - logger
$(EASYLOGGING) :  $(INCLUDE_PATH)easylogging++.h


# Option 2: Generate the documentation
documentation :
	$(DOC_GENERATOR)

# Option 3: Clean up
clean :
	$(DELETE_RECURSIVE) $(DOC_PATH)html
	$(DELETE_RECURSIVE) bin/pcfgem
