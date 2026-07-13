GCC_OPTS += -O1 -flto

# 8 s @ mtime's fixed 1 us tick; adjust for sim
HEARTBEAT = 8000000
# characters to buffer before the loopback
BATCH = 5

CFLAGS += -DHEARTBEAT_US=$(HEARTBEAT) -DBATCH=$(BATCH)

# three flavors, one build flag each:
#   interrupt_idling: traps on,  busy NOP loop (interrupts preempt)
#   interrupt_wfi:    traps on,  WFI sleep (interrupt wakes + traps)
#   wakeup_wfi:       traps off, WFI wakes without trapping, main polls
TARGETS = test_interrupt_idling.elf test_interrupt_wfi.elf test_wakeup_wfi.elf

all: $(TARGETS)

test_interrupt_idling.elf: EXTRA_DEFINES := -DINTERRUPT_IDLING
test_interrupt_wfi.elf:    EXTRA_DEFINES := -DINTERRUPT_WFI
test_wakeup_wfi.elf:       EXTRA_DEFINES := -DWAKEUP_WFI

include ../Makefile.inc
