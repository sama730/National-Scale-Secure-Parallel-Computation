
CC = g++

CPPFLAGS := -std=c++11 -pthread -O3 -lssl -lcrypto -lpthread -maes -pthread -mrdseed -rdynamic /usr/local/lib/librelic.so -lboost_system -lgmp /usr/local/lib/libemp-tool.so -Wl,-rpath,/usr/local/lib

SRC = FourParty.cpp TwoPartyMaskedEvaluation/MaskedEvaluation.cpp TwoPartyMaskedEvaluation/PreprocessingShareStorage.cpp TwoPartyMaskedEvaluation/PreprocessingShare.cpp TwoPartyMaskedEvaluation/SimplePreprocessingBuilder.cpp TwoPartyMaskedEvaluation/PreprocessingBuilder.cpp Circuit/Gate.cpp Circuit/ArithmeticCircuit.cpp Circuit/ArithmeticCircuitBuilder.cpp Circuit/LayeredArithmeticCircuitBuilder.cpp Circuit/LayeredArithmeticCircuit.cpp Circuit/PlainArithmeticCircuitEvaluator.cpp Utility/ISecureRNG.cpp Utility/CryptoUtility.cpp Utility/Commitment.cpp Utility/Range.cpp Utility/Communicator.cpp CrossCheck/PreprocessingParty.cpp CrossCheck/Player.cpp CrossCheck/CrossChecker.cpp CrossCheck/LeakyVetoProtocol.cpp

.phony: all clean

EXES = FourParty
all: $(EXES)
$(EXES): 
	$(CC) $(CPPFLAGS) $(SRC) -o FourParty

clean:
	rm -rf $(EXES)

