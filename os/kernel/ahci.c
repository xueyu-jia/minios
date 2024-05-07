#include "type.h"
#include "const.h"
#include "string.h"
#include "protect.h"
#include "proc.h"
#include "proto.h"
#include "memman.h"
#include "pagetable.h"
#include "ahci.h"

// 0: free;  1: waiting
PUBLIC volatile int sata_wait_flag = 1;
// 0 : no error;  1: error
PUBLIC volatile int sata_error_flag = 0;

PUBLIC HBA_MEM *HBA;

PUBLIC AHCI_INFO ahci_info
    [MAX_AHCI_NUM]; // 可能存在多个AHCI控制器，为了方便目前只支持1个，这里只用遍历到的第一个。

PRIVATE int  check_type(HBA_PORT *port);
PRIVATE void probe_port(HBA_MEM *abar);
PRIVATE void sata_handler(int irq);
void         port_rebase(HBA_PORT *port);

PUBLIC int AHCI_init() // 遍历pci设备，找到AHCI  by qianglong	2022.5.17
{
    u32 bus, dev, fun, err_temp, sata_irq;
    memset(ahci_info, 0, sizeof(AHCI_INFO));
    u32 AHCI_cnt = 0;
    for (bus = 0; bus <= MAXBUS; bus++) {
        for (dev = 0; dev <= MAXDEV; dev++) {
            for (fun = 0; fun <= MAXFUN; fun++) {
                out_dword(
                    PCI_CONFIG_ADD,
                    mk_pci_add(
                        bus, dev, fun, 0)); // 0号寄存器为device——id和vendor——id
                if ((in_dword(PCI_CONFIG_DATA) & 0xffff)
                    != 0xffff) { // 如果vendorid合法,表示该位置存在设备
                    out_dword(
                        0xcf8,
                        mk_pci_add(
                            bus, dev, fun, 2)); // 2号寄存器为classcode,设备类别
                    if ((in_dword(PCI_CONFIG_DATA) >> 16)
                        == 0x0106) { // 判断是否为AHCI
                        // disp_str("  bus: ");
                        // disp_int(bus);
                        // disp_str("  dev: ");
                        // disp_int(dev);
                        // disp_str("  fun: ");
                        // disp_int(fun);
                        // disp_str("\n");
                        AHCI_cnt += 1;
                        // if(AHCI_cnt>MAX_AHCI_NUM)break;
                        ahci_info[AHCI_cnt - 1].achi_pos_pci.bus = bus;
                        ahci_info[AHCI_cnt - 1].achi_pos_pci.dev = dev;
                        ahci_info[AHCI_cnt - 1].achi_pos_pci.fun = fun;
                        out_dword(
                            PCI_CONFIG_ADD,
                            mk_pci_add(bus, dev, fun, 9)); // 9号寄存器为ABAR
                        disp_str("  ABAR: ");
                        ahci_info[AHCI_cnt - 1].ABAR =
                            in_dword(PCI_CONFIG_DATA);
                        disp_int(ahci_info[AHCI_cnt - 1].ABAR);

                        out_dword(
                            PCI_CONFIG_ADD, mk_pci_add(bus, dev, fun, 15));
                        disp_str("  interrupt: ");

                        disp_int(in_dword(PCI_CONFIG_DATA));
                        ahci_info[AHCI_cnt - 1].irq_info =
                            in_dword(PCI_CONFIG_DATA) & 0xffff;
                    }
                    // if ((in_dword(PCI_CONFIG_DATA)>>16)==0x0101)
                    // {
                    // 	disp_str(" idecontorler ");
                    // }
                }
            }
        }
    }
    if (AHCI_cnt == 0) {
        disp_str("no AHCI HBA!");
        return FALSE;
    }

    // 将AHCI基地址映射到线性地址
    int lin_hba = kmapping_phy(ahci_info[0].ABAR);
    // err_temp = lin_mapping_phy_nopid(	ahci_info[0].ABAR,  //线性地址
    // //add by visual 2016.5.9 									ahci_info[0].ABAR, //物理地址 									read_cr3(), 									PG_P |
    // PG_USS | PG_RWW,  //页目录的属性位（系统权限）			//edit by visual
    // 2016.5.26 									PG_P | PG_USS | PG_RWW); //页表的属性位（系统权限）
    // //edit by visual 2016.5.17 for(int pid = 0; pid <= NR_K_PCBS; pid++){
    // 	if(proc_table[pid].task.stat == READY){
    // 		err_temp |= lin_mapping_phy(	ahci_info[0].ABAR,  //线性地址					//add
    // by visual 2016.5.9 									ahci_info[0].ABAR, //物理地址 									pid, 									PG_P | PG_USS |
    // PG_RWW,  //页目录的属性位（系统权限）			//edit by visual
    // 2016.5.26 									PG_P | PG_USS | PG_RWW); //页表的属性位（系统权限）
    // 	}
    // }
    // int hd_service_pid = kern_get_pid_byname("hd_service");

    // err_temp |= lin_mapping_phy(	ahci_info[0].ABAR,  //线性地址					//add by
    // visual 2016.5.9 									ahci_info[0].ABAR, //物理地址 									hd_service_pid, 									PG_P |
    // PG_USS | PG_RWW,  //页目录的属性位（系统权限）			//edit by visual
    // 2016.5.26 									PG_P | PG_USS | PG_RWW); //页表的属性位（系统权限）

    // int task_tty_pid = kern_get_pid_byname("task_tty");

    // err_temp |= lin_mapping_phy(	ahci_info[0].ABAR,  //线性地址					//add by
    // visual 2016.5.9 									ahci_info[0].ABAR, //物理地址 									task_tty_pid, 									PG_P | PG_USS
    // | PG_RWW,  //页目录的属性位（系统权限）			//edit by visual
    // 2016.5.26 									PG_P | PG_USS | PG_RWW); //页表的属性位（系统权限）

    // int initial_pid = kern_get_pid_byname("initial");

    // err_temp |= lin_mapping_phy(	ahci_info[0].ABAR,  //线性地址					//add by
    // visual 2016.5.9 									ahci_info[0].ABAR, //物理地址 									initial_pid, 									PG_P | PG_USS
    // | PG_RWW,  //页目录的属性位（系统权限）			//edit by visual
    // 2016.5.26 									PG_P | PG_USS | PG_RWW); //页表的属性位（系统权限）

    if (lin_hba == 0)
    // if (err_temp != 0)
    {
        disp_color_str("kernel page Error:lin_mapping_phy_ahci", 0x74);
        return FALSE;
    }

    // disp_str("\nlin_mapping_phy_ahci");
    HBA = (HBA_MEM *)(lin_hba);

    ahci_info[0].is_AHCI_MODE = (HBA->ghc) >> 31;

    if (ahci_info[0].is_AHCI_MODE == 0) {
        disp_str("AHCI is not Enable!");
        return FALSE;
    }
    ahci_info[0].portnum = (HBA->cap) & 0xff;
    // disp_int(HBA->cap);
    // disp_int(HBA->ghc);

    probe_port(HBA);
    // disp_str("\nprobe_port");
    // disp_int(HBA->pi);
    u32 i = 0;
    for (i = 0; i < ahci_info[0].satadrv_num; i++) {
        port_rebase(&(HBA->ports[ahci_info[0].satadrv_atport[i]]));
    }

    HBA->ghc |= (1 << 1); // enable interrupt
    sata_irq  = ahci_info[0].irq_info & 0xff;
    if (sata_irq <= 16) {
        put_irq_handler(sata_irq, sata_handler);
        enable_irq(sata_irq);
    } else {
        disp_str("sata_irq is large than 16");
    }
    // disp_str("\nread write test:");
    // // u64 n=0x800;
    // u64 n=3000;
    // SATA_rdwt_test(1,n);
    // SATA_rdwt_test(0,n);
    // n+=1;
    // SATA_rdwt_test(1,n);
    // SATA_rdwt_test(0,n);
    // n+=1;
    // SATA_rdwt_test(1,n);
    // SATA_rdwt_test(0,n);
    // n=0x12bffffc;
    // // SATA_rdwt_test(1,n);
    // SATA_rdwt_test(0,n);
    // n+=1;
    // // SATA_rdwt_test(1,n);
    // SATA_rdwt_test(0,n);
    // n+=1;
    // // SATA_rdwt_test(1,n);
    // SATA_rdwt_test(0,n);
    // n=0x1000;
    // SATA_rdwt_test(1,n);
    // SATA_rdwt_test(0,n);
    // while(1);
    return TRUE;
}

/*
 * @brief   sata中断处理函数
 * @param 	irq
 * @details 只支持一个AHCI控制器ahci_info[0]，错误处理未完成
 * 			因为hd_service是单线程的，因此只支持一个端口的中断处理
 * 			必须先完成端口的清中断，再完成控制器的清中断
 * 			目前没加内存屏障，有出现bug的可能性
 */
PRIVATE void sata_handler(int irq) {
    u32 port_bitmap = HBA->is;

    if (port_bitmap == 0) {
        disp_str("SATA handler:No error but interrupt\n");
        return;
    }

    // 判断是哪个端口触发的中断
    int prot_num;
    for (prot_num = 0; prot_num <= MAXDEV; prot_num++) {
        if (port_bitmap & (1 << prot_num)) break;
    }
    // 中断处理
    u32 intr_status         = HBA->ports[prot_num].is;
    HBA->ports[prot_num].is = intr_status; // 端口清中断
    // disp_int((int)intr_status);
    if (intr_status == 0) {
        // debug
        // disp_str("\nsata_handler: PxIS = 0\n");
    } else if (intr_status & HBA_Port_ERROR) {
        // 发生错误
        disp_str("\nsata_handler:error\n");
        disp_int((int)intr_status);
        tf_err_rec(&(HBA->ports[prot_num])); // 错误处理
        sata_error_flag = 1;
        if (kernel_initial == 1){
            sata_wait_flag = 0; // 完成错误处理后唤醒进程
        }else{
            wakeup((void*)&sata_wait_flag);
        }
    } else if (intr_status & HBA_Port_COMPLETED) {
        // 数据读写完成
        sata_error_flag = 0;
        if (kernel_initial == 1){
            sata_wait_flag = 0;
        }else{
            wakeup((void*)&sata_wait_flag);
        }
    } else {
        // 其他错误, undo
        disp_str("\nPANIC:sata_handler:other error\n");
        disp_int((int)intr_status);
        sata_error_flag = 1;
    }
    HBA->is = port_bitmap; // 控制器清中断
    return;
}

// Detect attached SATA devices
PRIVATE void probe_port(HBA_MEM *abar) {
    // Search disk in implemented ports
    u32 pi                   = abar->pi;
    ahci_info[0].satadrv_num = 0;
    int i                    = 0;
    while (i < 30) {
        if (pi & 1) {
            int dt = check_type(&abar->ports[i]);
            if (dt == AHCI_DEV_SATA) {
                disp_str("\nSATA drive found at port :");
                disp_int(i);
                ahci_info[0].satadrv_num                                  += 1;
                ahci_info[0].satadrv_atport[ahci_info[0].satadrv_num - 1]  = i;
            } else if (dt == AHCI_DEV_SATAPI) {
                disp_str("\nSATAPI drive found at port :");
                disp_int(i);
            } else if (dt == AHCI_DEV_SEMB) {
                disp_str("\nSEMB drive found at port:");
                disp_int(i);
            } else if (dt == AHCI_DEV_PM) {
                disp_str("\nPM drive found at port: ");
                disp_int(i);
            } else {
            };
        }

        pi >>= 1;
        i++;
    }
}

// Check device type
PRIVATE int check_type(HBA_PORT *port) {
    u32 ssts = port->ssts;

    u8 ipm = (ssts >> 8) & 0x0F;
    u8 det = ssts & 0x0F;

    if (det != HBA_PORT_DET_PRESENT) // Check drive status
        return AHCI_DEV_NULL;
    if (ipm != HBA_PORT_IPM_ACTIVE) return AHCI_DEV_NULL;

    switch (port->sig) {
        case SATA_SIG_ATAPI:
            return AHCI_DEV_SATAPI;
        case SATA_SIG_SEMB:
            return AHCI_DEV_SEMB;
        case SATA_SIG_PM:
            return AHCI_DEV_PM;
        default:
            return AHCI_DEV_SATA;
    }
}

void start_cmd(HBA_PORT *port) {
    // Wait until CR (bit15) is cleared
    while (port->cmd & HBA_PxCMD_CR)
        ;

    // Set FRE (bit4) and ST (bit0)
    port->cmd |= HBA_PxCMD_FRE;
    port->cmd |= HBA_PxCMD_ST;
}

// Stop command engine
void stop_cmd(HBA_PORT *port) {
    // Clear ST (bit0)
    port->cmd &= ~HBA_PxCMD_ST;

    // Clear FRE (bit4)
    port->cmd &= ~HBA_PxCMD_FRE;

    // Wait until FR (bit14), CR (bit15) are cleared
    while (1) {
        if (port->cmd & HBA_PxCMD_FR) continue;
        if (port->cmd & HBA_PxCMD_CR) continue;
        break;
    }
    // port->is = 0;
    // port->ie = 0;
}

// AHCI port memory space initialization
void port_rebase(HBA_PORT *port) {
    // HBA->ghc=(u32)(1<<31);
    // HBA->ghc=(u32)(1<<0);
    // HBA->ghc=(u32)(1<<31);
    // HBA->ghc=(u32)(1<<1);

    stop_cmd(port); // Stop command engine
    port->serr = (u32)-1;
    port->is   = 0;
    port->ie   = 0;
    // Command header entry size = 32 bytes
    // Command list entry maxim count = 32 Command header
    // Command list maxim size = 32*32 = 1K per port
    u32 CL_BASE = phy_kmalloc(sizeof(HBA_CMD_HEADER) * 32); // phy_add
    // disp_str("\nCL_BASE:");disp_int(CL_BASE);
    memset((void *)K_PHY2LIN(CL_BASE), 0, 1024);
    port->clb  = CL_BASE;
    port->clbu = 0;

    // FIS entry size = 256 bytes per port
    u32 FIS_BASE = phy_kmalloc(256);
    // disp_str("\nFIS_BASE:");disp_int(FIS_BASE);
    memset((void *)K_PHY2LIN(FIS_BASE), 0, 256);
    port->fb  = FIS_BASE;
    port->fbu = 0;

    // Command table size = 256*32 = 8K per port
    HBA_CMD_HEADER *cmdheader = (HBA_CMD_HEADER *)K_PHY2LIN(port->clb);
    u32             CT_BASE[32];

    for (int i = 0; i < 32; i++) {
        cmdheader[i].prdtl = 8; // 8 prdt entries per command table
                                // 256 bytes per command table, 64+16+48+16*8
        CT_BASE[i] = phy_kmalloc(sizeof(HBA_CMD_TBL));
        // disp_str("\nCT_BASE:");disp_int(CT_BASE[i]);
        cmdheader[i].ctba  = CT_BASE[i];
        cmdheader[i].ctbau = 0;
        memset((void *)K_PHY2LIN(cmdheader[i].ctba), 0, sizeof(HBA_CMD_TBL));
    }
    port->is = (u32)-1;
    port->ie = (u32)-1;
    start_cmd(port); // Start command engine
}

PUBLIC u32 identity_SATA(HBA_PORT *port, u8 *buf) {
    // u8* buf=K_PHY2LIN(do_kmalloc(1024));
    port->is = (u32)-1; // Clear pending interrupt bits
    int spin = 0;       // Spin lock timeout counter
    int slot = find_cmdslot(port);
    if (slot == -1) return 0;

    HBA_CMD_HEADER *cmdheader  = (HBA_CMD_HEADER *)K_PHY2LIN(port->clb);
    cmdheader                 += slot;
    cmdheader->cfl   = sizeof(FIS_REG_H2D) / sizeof(u32); // Command FIS size
    cmdheader->w     = 0;                                 // Read from device
    cmdheader->c     = 1;
    cmdheader->prdbc = 0;

    cmdheader->prdtl = 1; // PRDT entries count

    HBA_CMD_TBL *cmdtbl = (HBA_CMD_TBL *)K_PHY2LIN(cmdheader->ctba);
    memset(
        cmdtbl,
        0,
        sizeof(HBA_CMD_TBL) + (cmdheader->prdtl - 1) * sizeof(HBA_PRDT_ENTRY));
    cmdtbl->prdt_entry[0].dba = (u32)K_LIN2PHY(buf);
    cmdtbl->prdt_entry[0].dbc =
        512
        - 1; // this value should always be set to 1 less than the actual value
    cmdtbl->prdt_entry[0].i = 0;

    // Setup command
    FIS_REG_H2D *cmdfis = (FIS_REG_H2D *)(&cmdtbl->cfis);
    cmdfis->fis_type    = FIS_TYPE_REG_H2D;
    cmdfis->c           = 1; // Command
    cmdfis->command     = ATA_CMD_IDENTIFY;

    // The below loop waits until the port is no longer busy before issuing a
    // new command
    while ((port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin < 1000000) {
        spin++;
    }
    if (spin == 1000000) {
        disp_str("Port is hung\n");
        return FALSE;
    }

    // Wait for completion
    // while (1)
    // {
    // 	// In some longer duration reads, it may be helpful to spin on the DPS
    // bit
    // 	// in the PxIS port field as well (1 << 5)
    // 	if (((port->ci & (1<<slot)) == 0)&&(cmdheader->prdbc >= 512)) {
    // 		// disp_str("\nidentity_success,transfer byte
    // count:");disp_int(cmdheader->prdbc);
    // 		// disp_str("\nport_is:");disp_int(port->is);
    // 		break;
    // 	}
    // }
    if (kernel_initial == 1) {
        port->ci = 1 << slot; // Issue command
        while (sata_wait_flag)
            ;
        sata_wait_flag = 1;
    } else {
        disable_int();
        port->ci = 1 << slot; // Issue command
        wait_event((void*)&sata_wait_flag, 0);
        enable_int();
    }

    // if (port->is & HBA_PxIS_TFES)
    // {
    // 	// disp_str("Read disk error,restart port\n");
    // 	port->cmd &= ~HBA_PxCMD_ST;//restart port
    // 	spin=0;
    // 	while(1)
    // 	{
    // 		spin++;
    // 		if(spin>1000)break;
    // 	}
    // 	port->cmd |= HBA_PxCMD_ST;
    // 	return FALSE;
    // }
    return 1;
}

// Find a free command list slot
PUBLIC int find_cmdslot(HBA_PORT *port) {
    // If not set in SACT and CI, the slot is free
    u32 slots = (port->sact | port->ci);
    for (int i = 0; i < 32; i++) {
        if ((slots & 1) == 0) return i;
        slots >>= 1;
    }
    disp_str("Cannot find free command list entry\n");
    return -1;
}

PUBLIC void tf_err_rec(HBA_PORT *port) { // task erro recovry
    stop_cmd(port);
    start_cmd(port);
}