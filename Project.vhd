LIBRARY ieee;
USE ieee.std_logic_1164.all;

ENTITY Project IS
	PORT(
		clock, reset : IN std_logic;
		button_i : IN std_logic;
		scl, sda : INOUT std_logic;
		cs_n, sdo : OUT std_logic;
		display1_o, display2_o, display3_o, display4_o, display5_o, display6_o : OUT std_logic_vector(7 DOWNTO 0);
		leds_o : OUT std_logic_vector(9 DOWNTO 0)
	);
END Project;

ARCHITECTURE STRUCTURE OF Project IS

	COMPONENT Project_sys IS
		PORT (
						button_export     : in    std_logic                     := 'X'; -- export
			clk_clk           : in    std_logic                     := 'X'; -- clk
			i2c_scl_pad_io    : inout std_logic                     := 'X'; -- scl_pad_io
			i2c_sda_pad_io    : inout std_logic                     := 'X'; -- sda_pad_io
			leds_export       : out   std_logic_vector(9 downto 0);         -- export
			reset_reset_n     : in    std_logic                     := 'X'; -- reset_n
			seven_segs_export : out   std_logic_vector(23 downto 0)         -- export
		);
	END COMPONENT Project_sys;
	
	COMPONENT Display IS
		PORT(
			dsp_i : IN std_logic_vector(4 DOWNTO 0);
			dsp_o : OUT std_logic_vector(7 DOWNTO 0)
		);
	END COMPONENT Display;
	
	COMPONENT CustomDisplay IS
	PORT(
		dsp_i : IN std_logic_vector(2 DOWNTO 0);
		dsp_o : OUT std_logic_vector(7 DOWNTO 0)
	);
	END COMPONENT CustomDisplay;

	--SIGNAL DECLARATION
	signal seven_segs_s : std_logic_vector(23 DOWNTO 0);
	
	--INSTANTIATION AND WIRING	
	BEGIN
		Project_sys_inst : Project_sys PORT MAP(button_i, clock, scl, sda, leds_o, reset, seven_segs_s) ;
		Display1 : Display PORT MAP(seven_segs_s(4 DOWNTO 0),display1_o);
		Display2 : Display PORT MAP(seven_segs_s(9 DOWNTO 5),display2_o);
		Display3 : Display PORT MAP(seven_segs_s(14 DOWNTO 10),display3_o);
		Display4 : CustomDisplay PORT MAP(seven_segs_s(17 DOWNTO 15),display4_o);
		Display5 : CustomDisplay PORT MAP(seven_segs_s(20 DOWNTO 18),display5_o);
		Display6 : CustomDisplay PORT MAP(seven_segs_s(23 DOWNTO 21),display6_o);
		
		--Setup I2C
		cs_n <= '1';
		sdo <= '1';
END STRUCTURE;