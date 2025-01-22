library ieee;
Use ieee.std_logic_1164.all;

ENTITY CustomDisplay IS
	PORT(
		dsp_i : IN std_logic_vector(2 DOWNTO 0);
		dsp_o : OUT std_logic_vector(7 DOWNTO 0)
	);
END CustomDisplay;

ARCHITECTURE Behaviour OF CustomDisplay IS
	BEGIN
		with dsp_i select
			dsp_o <=    "10110111" when "001", -- =
							"10001001" when "010", -- X
							"10010001" when "011", -- Y
							"10100100" when "100", -- Z
							"10111111" when "101", -- -

							"11111111" when others;
	END Behaviour;
	