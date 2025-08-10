// Enumerations for interrupts and their sources

// CPU interrupts (counting from 1 because legacy)
enum Interrupt {
	INT_IRQ = 1,
	INT_FIRQ,
	INT_NMI
};

// Interrupt sources keep track of their own state.
// NMI is its own source and always uses this.
// IS_NMI and IS_IRQ automatically clear.
enum InterruptSource {
	IS_NMI = 0,
	IS_PIA0_HSYNC,
	IS_PIA0_VSYNC,
	IS_PIA1_CD,
	IS_PIA1_CART,
	IS_GIME,
	IS_PAK_IRQ,
	IS_MAX
};

