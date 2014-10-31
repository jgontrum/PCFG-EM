
# Values for MacOSX & Linux
CPPCOMPILER         = clang++
COMPILER_FLAGS      = -O2 -std=c++11 
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
build : $(SRC_PATH)main.cpp $(HEADERFILES)
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
