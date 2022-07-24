CXX = g++
CXXFLAGS = -Wall -g

lexan.o: lexan.cpp lexan.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
rpn.o: rpn.cpp rpn.hpp bot.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
syntan.o: syntan.cpp syntan.hpp lexan.hpp bot.hpp rpn.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
bot.o: bot.cpp bot.hpp syntan.hpp rpn.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
bot: main.cpp bot.o rpn.o syntan.o lexan.o
	$(CXX) $(CXXFLAGS) $^ -o $@
