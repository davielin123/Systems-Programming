all: bidding_system.c bidding_system_EDF.c customer.c
	gcc bidding_system.c -o bidding_system
	gcc bidding_system_EDF.c -o bidding_system_EDF
	gcc customer.c -o customer
	
clean:
	rm -f bidding_system bidding_system_EDF customer