// Enumerations for interrupts and their sources

// CPU interrupts (counting from 1 because legacy)
// The CPU automatically clears NMI
// Pakinterface uses INT_NONE to clear IRQ
enum Interrupt {
	INT_NONE = 0,
	INT_IRQ = 1,
	INT_FIRQ,
	INT_NMI
};

// Interrupt sources keep track of their own state.
// NMI is its own source and always uses this.
// PAK_IRQ is only used to Assert IRQ
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

