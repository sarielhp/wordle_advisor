all: wordle_advisor

wordle_advisor: wordle_advisor.C
	g++ -Wall -o wordle_advisor wordle_advisor.C

