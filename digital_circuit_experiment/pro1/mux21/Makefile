.PHONY: sim
sim:
	#$(call git_commit, "sim RTL") # DO NOT REMOVE THIS LINE!!!
	@echo "Write this Makefile by your self."
	verilator --cc --exe --build -j 0 -Wall sim.cpp mux21.v --trace
	./obj_dir/Vmux21
