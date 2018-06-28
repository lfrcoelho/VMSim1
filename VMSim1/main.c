//
//  main.c
//  VMSim1
//
//  Created by lcoelho on 19/06/2018.
//  Copyright © 2018 lcoelho. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define FILE_ROW_SIZE   5
#define FILE_MAX_ROWS   500

// PRICING
#define PRICE_MEM_8     0
#define PRICE_MEM_16    0
#define PRICE_MEM_32    0
#define PRICE_MEM_64    0
#define PRICE_MEM_128   0

#define PRICE_CPU    0

#define PRICE_BASE_SERVER   0

// LOCAL DISK DATA
int local_disk_flag = 0;
#define MAX_LOCAL_DISK_SIZE 7680

// TEMPLATES
#define SIZE_OF_TEMPLATES_ROW    5
#define TEMPLATE_FILE_MAX_ROWS  100
int template_array[TEMPLATE_FILE_MAX_ROWS][SIZE_OF_TEMPLATES_ROW];  // EACH ROW IN THE TEMPLATE ARRAY HAS TEMPLATE_NO, CORES, DIMM_SIZE, DIMM_QUANT, DISK
int total_number_of_templates;

// MEMORY DATA
#define SIZE_OF_MEM_DIMM_ARRAY_ROW  5
int mem_dimm_array[SIZE_OF_MEM_DIMM_ARRAY_ROW];
int mem_dimm_array_sizes[SIZE_OF_MEM_DIMM_ARRAY_ROW] = {8,16,32,64,128};
#define MAX_DIMM_SLOTS_PER_SERVER   24

// CPU DATA
int cpu_cores_per_server = 56;

// APP DATA
int app_anti_affinity = 0;

// MAIN FILE DATA
int input_data[FILE_MAX_ROWS][FILE_ROW_SIZE];
int input_rows, input_columns;

// VM DATA
int *vm_array; // EACH ROW IN THE VM_ARRAY HAS: APP_NO, VM_NO, NO_VCPUS, MEM_SIZE, DISK_SIZE, SERVER_NO
#define SIZE_OF_VM_ARRAY_ROW    6
int number_of_vms, total_number_of_pcores;

// SERVER DATA
int *server_array; // EACH ROW IN THE SERVER_ARRAY HAS: AVAILABLE_PCORES, OCCUP_MEM, OCCUP_DISK, DIMM_SIZE, DIMM_NO, TEMPLATE_NO
#define SIZE_OF_SERVER_ARRAY_ROW    6
int total_number_of_servers;
int total_number_of_sla_servers;
#define SERVER_BUFFER_SLA_THRESHOLD    15

void Add_Server()
{
    int *aux_ptr;
    int i;
    
    aux_ptr = (int *) malloc(sizeof(int) * SIZE_OF_SERVER_ARRAY_ROW * (total_number_of_servers + 1));
    
    for (i = 0; i < total_number_of_servers; i++)
    {
        aux_ptr[i * SIZE_OF_SERVER_ARRAY_ROW] = server_array[i * SIZE_OF_SERVER_ARRAY_ROW];
        aux_ptr[i * SIZE_OF_SERVER_ARRAY_ROW + 1] = server_array[i * SIZE_OF_SERVER_ARRAY_ROW + 1];
        aux_ptr[i * SIZE_OF_SERVER_ARRAY_ROW + 2] = server_array[i * SIZE_OF_SERVER_ARRAY_ROW + 2];
        aux_ptr[i * SIZE_OF_SERVER_ARRAY_ROW + 3] = server_array[i * SIZE_OF_SERVER_ARRAY_ROW + 3];
        aux_ptr[i * SIZE_OF_SERVER_ARRAY_ROW + 4] = server_array[i * SIZE_OF_SERVER_ARRAY_ROW + 4];
        aux_ptr[i * SIZE_OF_SERVER_ARRAY_ROW + 5] = server_array[i * SIZE_OF_SERVER_ARRAY_ROW + 5];
    }
    total_number_of_servers++;
    i = total_number_of_servers - 1;
    aux_ptr[i * SIZE_OF_SERVER_ARRAY_ROW] = cpu_cores_per_server;
    aux_ptr[i * SIZE_OF_SERVER_ARRAY_ROW + 1] = 0;
    aux_ptr[i * SIZE_OF_SERVER_ARRAY_ROW + 2] = 0;
    aux_ptr[i * SIZE_OF_SERVER_ARRAY_ROW + 3] = 0;
    aux_ptr[i * SIZE_OF_SERVER_ARRAY_ROW + 4] = 0;
    aux_ptr[i * SIZE_OF_SERVER_ARRAY_ROW + 5] = -1;
    
    //free(server_array);
    server_array = aux_ptr;
}

int VM_Array_Sort(int pos1, int pos2)
{
    int aux1;
    if (vm_array[pos1 * SIZE_OF_VM_ARRAY_ROW + 2] < vm_array[pos2 * SIZE_OF_VM_ARRAY_ROW + 2])
    {
        for (int i = 0; i < SIZE_OF_VM_ARRAY_ROW; i++)
        {
            aux1 = vm_array[pos1 * SIZE_OF_VM_ARRAY_ROW + i];
            vm_array[pos1 * SIZE_OF_VM_ARRAY_ROW + i] = vm_array[pos2 * SIZE_OF_VM_ARRAY_ROW + i];
            vm_array[pos2 * SIZE_OF_VM_ARRAY_ROW + i] = aux1;
        }
        return 1;
    }
    return 0;
}

void VM_Distribute()
{
    int k, aux1;
    int flag;
    
    for (int i = 0; i < number_of_vms; i++)
    {
        k = 0;
        aux1 = 0;
        while (aux1 == 0)
        {
            // LOOK FOR A SERVER WITH AVAILABLE CORES
            if (server_array[k * SIZE_OF_SERVER_ARRAY_ROW] >= (vm_array[i * SIZE_OF_VM_ARRAY_ROW + 2] / 2))
            {
                // CHECK ANTI-AFFINITY
                flag = 0;
                if (app_anti_affinity == 1)
                {
                    // Check if other VMs of the same APP are on the same server
                    for (int j = 0; j < number_of_vms; j++)
                    {
                        if (j != i)
                        {
                            if ((vm_array[i * SIZE_OF_VM_ARRAY_ROW] == vm_array[j * SIZE_OF_VM_ARRAY_ROW]) && (vm_array[j * SIZE_OF_VM_ARRAY_ROW + 5] == k))
                            {
                                flag = 1;
                                printf("ANTI-AFFINITY RULE....SERV.%d VM.%d APP.%d\n", k, vm_array[i * SIZE_OF_VM_ARRAY_ROW + 1], vm_array[i * SIZE_OF_VM_ARRAY_ROW]);
                                j = number_of_vms;
                            }
                        }
                    }
                }
                
                if (flag == 0)
                {
                    // CHECK MEMORY AVAILABILITY
                    if (((128 * MAX_DIMM_SLOTS_PER_SERVER - server_array[k * SIZE_OF_SERVER_ARRAY_ROW + 1]) - vm_array[i * SIZE_OF_VM_ARRAY_ROW + 3]) < 0)
                    {
                        // NOT ENOUGH MEMORY ON THE SERVER
                        flag = 1;
                        printf("SERVER.%d NOT ENOUGH MEMORY FOR APP.%d VM.%d\n", k, vm_array[i * SIZE_OF_VM_ARRAY_ROW], vm_array[i * SIZE_OF_VM_ARRAY_ROW + 1]);
                    }
                    // CHECK FOR LOCAL DISK AVAILABILITY
                    if (((2 * MAX_LOCAL_DISK_SIZE - server_array[k * SIZE_OF_SERVER_ARRAY_ROW + 2]) - vm_array[i * SIZE_OF_VM_ARRAY_ROW + 4]) < 0)
                    {
                        // NOT ENOUGH DISK ON THE SERVER
                        flag = 1;
                        printf("SERVER.%d NOT ENOUGH DISK FOR APP.%d VM.%d\n", k, vm_array[i * SIZE_OF_VM_ARRAY_ROW], vm_array[i * SIZE_OF_VM_ARRAY_ROW + 1]);
                    }
                    
                    
                    else
                    {
                        vm_array[i * SIZE_OF_VM_ARRAY_ROW + 5] = k;
                        server_array[k * SIZE_OF_SERVER_ARRAY_ROW] -= (vm_array[i * SIZE_OF_VM_ARRAY_ROW + 2] / 2); // Subtract available pCores
                        /* if (k == 2)
                         {
                         printf("VM.%d APP.%d...LEFT WITH %d pCPUs\n", vm_array[i * SIZE_OF_VM_ARRAY_ROW + 1], vm_array[i * SIZE_OF_VM_ARRAY_ROW], server_array[k * SIZE_OF_SERVER_ARRAY_ROW]);
                         }*/
                        server_array[k * SIZE_OF_SERVER_ARRAY_ROW + 1] += vm_array[i * SIZE_OF_VM_ARRAY_ROW + 3];   // Add memory
                        server_array[k * SIZE_OF_SERVER_ARRAY_ROW + 2] += vm_array[i * SIZE_OF_VM_ARRAY_ROW + 4];   // Add Disk
                        aux1 = 1;
                    }
                }
                if (flag == 1)
                {
                    k++;
                    if (k == total_number_of_servers)
                    {
                        printf("COULD NOT FIND A SERVER FOR VM%d APP%d. Adding another one!\n", vm_array[i * SIZE_OF_VM_ARRAY_ROW + 1], vm_array[i * SIZE_OF_VM_ARRAY_ROW]);
                        Add_Server();
                    }
                }
            }
            else
            {
                k++;
                if (k == total_number_of_servers)
                {
                    printf("COULD NOT FIND A SERVER FOR VM%d APP%d\n", vm_array[i * SIZE_OF_VM_ARRAY_ROW + 1], vm_array[i * SIZE_OF_VM_ARRAY_ROW]);
                    Add_Server();
                }
            }
        }
    }
}

int GetDIMM(int current_index)
{
    int index;
    int aux1 = 0;
    
    index = current_index;
    while (aux1 == 0)
    {
        index++;
        if ((index < 0) || (index >= SIZE_OF_MEM_DIMM_ARRAY_ROW))
        {
            return -1;
        }
        
        if (mem_dimm_array[index] != 0)
        {
            aux1 = 1;
        }
    }
    return mem_dimm_array[index];
}

void DIMM_Distribute(int server_no)
{
    int a;
    int dimm_index = -1;
    int dimm_size;
    int aux1 = 0;
    
    while (aux1 == 0)
    {
        dimm_size = GetDIMM(dimm_index);
        if (dimm_size != -1)
        {
            a = (int) ceil((double)server_array[server_no * SIZE_OF_SERVER_ARRAY_ROW + 1] / (double)dimm_size);
            if (a%2 != 0) a++;
            if (a > MAX_DIMM_SLOTS_PER_SERVER)
            {
                dimm_index++;
            }
            else
            {
                //printf("SERVER.%d DIMM_SIZE.%d DIMM_NO.%d\n", server_no, dimm_size, a);
                server_array[server_no * SIZE_OF_SERVER_ARRAY_ROW + 3] = dimm_size;
                server_array[server_no * SIZE_OF_SERVER_ARRAY_ROW + 4] = a;
                return;
            }
        }
        else
        {
            //printf("SERVER.%d TOO MUCH MEMORY...COULD NOT FIND A DIMM SIZE\n", server_no);
            return;
        }
    }
}

void Check_Server_Template(int srv_no)
{
    int valid_templates[total_number_of_templates];
    
    int server_cpus = cpu_cores_per_server - server_array[srv_no * SIZE_OF_SERVER_ARRAY_ROW];
    int server_memory = server_array[srv_no * SIZE_OF_SERVER_ARRAY_ROW + 1];
    int server_disk = server_array[srv_no * SIZE_OF_SERVER_ARRAY_ROW + 2];
    
    for (int i = 0; i < total_number_of_templates; i++)
    {
        if ((server_cpus <= template_array[i][1]) && (server_memory <= (template_array[i][2] * template_array[i][3])))
        {
            if (local_disk_flag == 1)
            {
                if (server_disk <= template_array[i][4])
                {
                    valid_templates[i] = 1;
                    //printf("SERVER.%d: TEMPLATE.%d\n", srv_no, i);
                }
                else valid_templates[i] = 0;
            }
            else
            {
                valid_templates[i] = 1;
                //printf("SERVER.%d: TEMPLATE.%d\n", srv_no, i);
            }
        }
        else
        {
            valid_templates[i] = 0;
        }
    }
    
    /// FIND THE BEST TEMPLATE
    int best_template = -1;
    for (int i = 0; i < total_number_of_templates; i++)
    {
        if (valid_templates[i] == 1)
        {
            if (best_template == -1)
            {
                best_template = i;
            }
            else
            {
                if (template_array[i][1] < template_array[best_template][1])    // CHECK CPU
                {
                    best_template = i;
                }
                else if (template_array[i][1] == template_array[best_template][1])
                {
                    if ((template_array[i][2] * template_array[i][3]) < (template_array[best_template][2] * template_array[best_template][3]))  // CHECK MEMORY
                    {
                        best_template = i;
                    }
                    else if ((template_array[i][2] * template_array[i][3]) == (template_array[best_template][2] * template_array[best_template][3]))
                    {
                        if (local_disk_flag == 1)   // CHECK DISK
                        {
                            if (template_array[i][4] < template_array[best_template][4])
                            {
                                best_template = i;
                            }
                        }
                    }
                }
            }
        }
    }
    if (best_template == -1) // NO TEMPLATE FOUND
    {
        printf("SERVER.%d\tNO VALID TEMPLATE FOUND!\n", srv_no);
    }
    else
    {
        printf("SERVER.%d\tTEMPLATE.%d\tCPU.%d\tDIMM_SIZE.%d\tDIMM_NO.%d\tDISK_SIZE.%d\t\n", srv_no, template_array[best_template][0], template_array[best_template][1], template_array[best_template][2], template_array[best_template][3], template_array[best_template][4]);
    }
}

void Template_Servers()
{
    for (int i = 0; i < (total_number_of_servers - total_number_of_sla_servers); i++)
    {
        Check_Server_Template(i);
    }
}

int main(int argc, const char * argv[])
{
    FILE *fp;
    FILE *fp2;
    
///////////////////////////////////////////////////////////////// FILE HANDLING
    
    if (argc != 5)
    {
        printf("Usage: %s <FILE NAME> <ANTI AFFINITY 0/1> <SERVER_CORES> <LOCAL DISK 0/1>\n", argv[0]);
        return -1;
    }
    
    app_anti_affinity = atoi(argv[2]);
    if ((app_anti_affinity != 0) && (app_anti_affinity != 1))
    {
        printf("Usage: %s <FILE NAME> <ANTI AFFINITY 0/1> <SERVER_CORES> <LOCAL DISK 0/1>\n", argv[0]);
        return -1;
    }
    
    local_disk_flag = atoi(argv[4]);
    if ((local_disk_flag != 0) && (local_disk_flag != 1))
    {
        printf("Usage: %s <FILE NAME> <ANTI AFFINITY 0/1> <SERVER_CORES> <LOCAL DISK 0/1>\n", argv[0]);
        return -1;
    }
    
    cpu_cores_per_server = atoi(argv[3]);
    if (cpu_cores_per_server <= 0)
    {
        printf("Usage: %s <FILE NAME> <ANTI AFFINITY 0/1> <SERVER_CORES> <LOCAL DISK 0/1>\n", argv[0]);
        return -1;
    }
    
    fp = fopen(argv[1], "r");
    if (fp == NULL)
    {
        printf("INPUT FILE OPENING ERROR!\nABORTING...\n");
        return -1;
    }
    
    input_rows = 0; input_columns = 0;
    while (fscanf(fp, "%d", &input_data[input_rows][input_columns]) == 1)
    {
        input_columns++;
        if (input_columns == FILE_ROW_SIZE)
        {
            input_rows++;
            input_columns = 0;
        }
        if (input_rows == FILE_MAX_ROWS) break;
    }
    if ((!feof(fp)) || (input_columns != 0))
    {
        printf ("PROBLEMS IN INPUT FILE...\nABORTING...\n");
        fclose(fp);
        return -1;
    }
    fclose(fp);
    
    // TEMPLATES FILE OPENING
    if (local_disk_flag == 0)
    {
        fp = fopen("templates-nd.txt", "r");
    }
    else
    {
        fp = fopen("templates-nv.txt", "r");
    }
    if (fp == NULL)
    {
        printf("INPUT TEMPLATE FILE OPENING ERROR!\nABORTING...\n");
        return -1;
    }
    
    int aux_rows, aux_columns;
    aux_rows = 0; aux_columns = 0;
    while (fscanf(fp, "%d", &template_array[aux_rows][aux_columns]) == 1)
    {
        aux_columns++;
        if (aux_columns == SIZE_OF_TEMPLATES_ROW)
        {
            aux_rows++;
            aux_columns = 0;
        }
        if (aux_rows == TEMPLATE_FILE_MAX_ROWS) break;
    }
    if ((!feof(fp)) || (aux_columns != 0))
    {
        printf ("PROBLEMS IN TEMPLATE FILE...\nABORTING...\n");
        fclose(fp);
        return -1;
    }
    fclose(fp);
    total_number_of_templates = aux_rows;
    
    // MEM_DIMMS FILE OPENING
    
    fp2 = fopen("mem_dimms.txt", "r");
    if (fp2 == NULL)
    {
        printf("MEM_DIMMS FILE OPENING ERROR!\nABORTING...\n");\
        fclose(fp);
        return -1;
    }

    for (int i = 0 ; i < SIZE_OF_MEM_DIMM_ARRAY_ROW; i++)
    {
        if (fscanf(fp2, "%d", &mem_dimm_array[i]) != 1)
        {
            printf("MEM_DIMMS FILE SPEC ERROR!\nABORTING...\n");
            fclose(fp2);
            return -1;
        }
        if (mem_dimm_array[i] == 1)
        {
            mem_dimm_array[i] = mem_dimm_array_sizes[i];
        }
        else if (mem_dimm_array[i] != 0)
        {
            printf("MEM_DIMMS FILE SPEC ERROR!\nABORTING...\n");
            fclose(fp); fclose(fp2);
            return -1;
        }
    }
    fclose(fp2);
    
/*    for (int i = 0 ; i < SIZE_OF_MEM_DIMM_ARRAY_ROW; i++)
    {
        printf("%d\n", mem_dimm_array[i]);
    }*/

    printf("\n==================== INITIAL DATA ==============================\n\n");
    printf("INPUT FILE...%s\n\n", argv[1]);
    printf("NUMBER OF pCORES PER SERVER...%d\n", cpu_cores_per_server);
    printf("ANTI AFFINITY RULE...%d\n", app_anti_affinity);
    printf("LOCAL DISK ENABLED...%d\n", local_disk_flag);
    
///////////////////////////////////////////////////////////////// VM ARRAY PROCESSING
    
    number_of_vms = 0;
    for (int i = 0; i < input_rows; i++)
    {
        number_of_vms += input_data[i][1];
    }
    printf("TOTAL NUMBER OF VMs...%d\n", number_of_vms);
    
    vm_array = (int *) malloc(sizeof(int) * SIZE_OF_VM_ARRAY_ROW * number_of_vms);
    int k = 0;
    for (int i = 0; i < input_rows; i++)
    {
        for (int j = 0; j < input_data[i][1]; j++)
        {
            vm_array[k * SIZE_OF_VM_ARRAY_ROW] = input_data[i][0];  // GET APP_NO
            vm_array[k * SIZE_OF_VM_ARRAY_ROW + 1] = j + 1;  // GET VM_NO
            vm_array[k * SIZE_OF_VM_ARRAY_ROW + 2] = input_data[i][2];  // GET NO_VCPUs
            //Check if vCPUs can fit into the Server CPU
            if (vm_array[k * SIZE_OF_VM_ARRAY_ROW + 2] > (cpu_cores_per_server * 2))
            {
                printf("ERROR...SERVER CPU MODEL CANNOT ACCOMMODATE SOME VM VCPU SIZES!\n");
                return -1;
            }
            vm_array[k * SIZE_OF_VM_ARRAY_ROW + 3] = input_data[i][3];  // GET MEM_SIZE
            vm_array[k * SIZE_OF_VM_ARRAY_ROW + 4] = input_data[i][4];  // GET DISK_SIZE
            vm_array[k * SIZE_OF_VM_ARRAY_ROW + 5] = -1;  // SERVER_NO
            k++;
        }
    }

/*
    for (int i = 0; i < number_of_vms; i++)
    {
        printf("%d. APP%d VM%d VCPU.%d MEM.%d DISK.%d %d\n", i, vm_array[i * SIZE_OF_VM_ARRAY_ROW], vm_array[i * SIZE_OF_VM_ARRAY_ROW + 1], vm_array[i * SIZE_OF_VM_ARRAY_ROW + 2], vm_array[i * SIZE_OF_VM_ARRAY_ROW + 3], vm_array[i * SIZE_OF_VM_ARRAY_ROW + 4], vm_array[i * SIZE_OF_VM_ARRAY_ROW + 5]);
    }*/
    
    // VM ARRAY SORTING (DESCENDING PER NO_CORES)
    k = 0;
    int aux;
    while (k < (number_of_vms - 1))
    {
        aux = k + 1;
        while (aux < number_of_vms)
        {
            VM_Array_Sort(k, aux);
            aux++;
        }
        k++;
    }

    printf("\n==================== SORTED TABLE OF VMS ==============================\n\n");
    for (int i = 0; i < number_of_vms; i++)
     {
     printf("%d. APP%d VM%d VCPU.%d MEM.%d DISK.%d\n", i, vm_array[i * SIZE_OF_VM_ARRAY_ROW], vm_array[i * SIZE_OF_VM_ARRAY_ROW + 1], vm_array[i * SIZE_OF_VM_ARRAY_ROW + 2], vm_array[i * SIZE_OF_VM_ARRAY_ROW + 3], vm_array[i * SIZE_OF_VM_ARRAY_ROW + 4]);
     }
    
    total_number_of_pcores = 0;
    for (int i = 0; i < number_of_vms; i++)
    {
        total_number_of_pcores += (vm_array[i * SIZE_OF_VM_ARRAY_ROW + 2] / 2);
    }
    
    // INITIAL SERVER INITIATION
    printf("\n==================== INITIAL SERVER CALCULATIONS ==============================\n\n");
    printf("TOTAL NUMBER OF pCORES...%d\n", total_number_of_pcores);
    
    total_number_of_servers = (int) ceil((double)total_number_of_pcores / (double)cpu_cores_per_server);
    printf("TOTAL INITIAL NUMBER OF SERVERS...%d\n", total_number_of_servers);
    
    server_array = (int *) malloc(sizeof(int) * SIZE_OF_SERVER_ARRAY_ROW * total_number_of_servers);
    for (int i = 0; i < total_number_of_servers; i++)
    {
        server_array[i * SIZE_OF_SERVER_ARRAY_ROW] = cpu_cores_per_server; // PCORES
        server_array[i * SIZE_OF_SERVER_ARRAY_ROW + 1] = 0; //OCC_MEM
        server_array[i * SIZE_OF_SERVER_ARRAY_ROW + 2] = 0; //OCC_DISK
        server_array[i * SIZE_OF_SERVER_ARRAY_ROW + 3] = 0; //DIMM_SIZE
        server_array[i * SIZE_OF_SERVER_ARRAY_ROW + 4] = 0; //DIMM_NO
        server_array[i * SIZE_OF_SERVER_ARRAY_ROW + 5] = -1; //TEMPLATE_NO
    }
    /*
    for (int i = 0; i < total_number_of_servers; i++)
    {
        printf("SERVER.%d pCPU_Left.%d MEM_OCC.%d DISK_OCC.%d\n", i, server_array[i * SIZE_OF_SERVER_ARRAY_ROW], server_array[i * SIZE_OF_SERVER_ARRAY_ROW + 1], server_array[i * SIZE_OF_SERVER_ARRAY_ROW + 2], server_array[i * SIZE_OF_SERVER_ARRAY_ROW + 3]);
    }*/
    
    VM_Distribute();
    
    printf("\n==================== VM TO SERVER ALLOCATION ==============================\n\n");
    
    for (int i = 0; i < number_of_vms; i++)
     {
     printf("%d. APP%d VM%d VCPU.%d MEM.%d DISK.%d \t\t-> SERVER.%d\n", i, vm_array[i * SIZE_OF_VM_ARRAY_ROW], vm_array[i * SIZE_OF_VM_ARRAY_ROW + 1], vm_array[i * SIZE_OF_VM_ARRAY_ROW + 2], vm_array[i * SIZE_OF_VM_ARRAY_ROW + 3], vm_array[i * SIZE_OF_VM_ARRAY_ROW + 4], vm_array[i * SIZE_OF_VM_ARRAY_ROW + 5]);
     }

    ///// SERVER BUFFER FOR SLA

    total_number_of_sla_servers = (int) ceil((double)total_number_of_servers / (double)SERVER_BUFFER_SLA_THRESHOLD);
    printf("SLA.......NEED TO ADD %d SERVERS\n", total_number_of_sla_servers);
    for (int i = 0; i < total_number_of_sla_servers; i++)
    {
        Add_Server();
    }
    
    printf("\n==================== RAW SERVER CONFIGURATION ==============================\n\n");
    printf("FINAL NUMBER OF SERVERS...%d\n\n", total_number_of_servers);
    for (int i = 0; i < total_number_of_servers; i++)
    {
        // ALLOCATE DIMMS TO SERVER
        DIMM_Distribute(i);
        printf("SERVER.%d\tpCPU_Left.%d\tDIMM_SIZE.%d\tDIMM_NO.%d\tMEM_OCC.%d\tDISK_OCC.%d\t", i, server_array[i * SIZE_OF_SERVER_ARRAY_ROW], server_array[i * SIZE_OF_SERVER_ARRAY_ROW + 3], server_array[i * SIZE_OF_SERVER_ARRAY_ROW + 4], server_array[i * SIZE_OF_SERVER_ARRAY_ROW + 1], server_array[i * SIZE_OF_SERVER_ARRAY_ROW + 2]);
        // M CPU CHECK
        if (server_array[i * SIZE_OF_SERVER_ARRAY_ROW + 1] >= (2 * 768)) printf("* xxxxM CPU NEEDED");
        printf("\n");
    }
    
    fclose(fp); fclose(fp2);
    printf("\nEND OF PROCESSING...\n");
    
    ///// TEMPLATE CONFIGURATION
    
    printf("\n==================== TEMPLATE CONFIGURATION (FOR NON SLA SERVERS) ==============================\n\n");
    Template_Servers();
    for (int i = 0; i < total_number_of_servers; i++)
    {
        
    }
    
/////////////////////////////////////////////////////////////////
    
    return 0;
}
