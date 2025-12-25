# KaraOS

A 64-bit x86-64 microkernel built from first principles. Designed around absolute modularity—each subsystem is architecturally isolated and independently testable.

## Features

- **Limine Bootloader** - UEFI/BIOS compatible bootstrapping via Limine protocol
- **Higher-Half Direct Map (HHDM)** - Efficient kernel memory management
- **Paging & Virtual Memory** - Full 4-level page table walk, TLB management
- **Physical Memory Manager** - Page allocator with fragmentation handling
- **Interrupt Handling** - IDT setup with exception & IRQ routing
- **GDT/Segmentation** - x86-64 descriptor tables
- **Modular Architecture** - Each subsystem independently replaceable

## Building

```bash
make iso
```

Outputs `bin/karaos` kernel image ready for ISO creation.

## Architecture

- **Kernel**: x86-64 long mode (64-bit)
- **Bootstrap**: Limine protocol (BIOS + UEFI)
- **Memory Model**: Identity-mapped bootloader regions, higher-half kernel mapping
- **Design Principle**: Do one thing well; interfaces between modules are minimal