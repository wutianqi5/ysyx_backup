.PHONY: sim
sim:
	#$(call git_commit, "sim RTL") # DO NOT REMOVE THIS LINE!!!
	@echo "Write this Makefile by your self."
	verilator --cc --exe --build -j 0 -Wall sim.cpp encoder42.v --trace
	./obj_dir/Vencoder42
