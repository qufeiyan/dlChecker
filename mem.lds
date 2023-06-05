__DLC_HEAP_SIZE = 0x100000;

SECTIONS {
	.dlc.heap : {
		_dlc_heap_start = .;
		. = ALIGN(4);

		. += __DLC_HEAP_SIZE;
		
		. = ALIGN(4);
		_dlc_heap_end = .;
	}

	PROVIDE(__DLC_HEAP_START = _dlc_heap_start);
	PROVIDE(__DLC_HEAP_END = _dlc_heap_end);
}

__DLC_MEMPOOL_SIZE = 0x1000000;
SECTIONS {
	.dlc.mempool : {
		_dlc_mempool_start = .;
		. = ALIGN(4);

		. += __DLC_MEMPOOL_SIZE;
		
		. = ALIGN(4);
		_dlc_mempool_end = .;
	}

	PROVIDE(__DLC_MEMPOOL_START = _dlc_mempool_start);
	PROVIDE(__DLC_MEMPOOL_END = _dlc_mempool_end);
}

INSERT AFTER .bss;
