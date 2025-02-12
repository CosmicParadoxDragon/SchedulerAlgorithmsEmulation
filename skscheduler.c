/* Author: Sohrab Kazak | gk3222
 * Date: 10/24/2023
 * Version: 1.0
 * Desc: This is the code for the CPU scheduling simulation
 * that we are doing as project 3 in the course CS 421.
 */
#include <stdlib.h>
#include <stdio.h>
#define NO_RUNNING_PROCESS 2
#define RUNNING_PROCESS_INCREMENT 1
/* Chart Building Resources */
char header_top_bar []= "┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓\n";
char header_bot_bar []= "┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛\n";
char top_bar []= "┏━━━━━━━━━━━━━━━┳━━━━━━━━━━━━━━━┳━━━━━━━━━━━━━━━┓\n";
char mid_bar []= "┣━━━━━━━━━━━━━━━╋━━━━━━━━━━━━━━━╋━━━━━━━━━━━━━━━┫\n";
char bot_bar []= "┗━━━━━━━━━━━━━━━┻━━━━━━━━━━━━━━━┻━━━━━━━━━━━━━━━┛\n";
char vertical_bar [] = "┃";

/* Simulation Parts */
struct sim_pcb_node {
    /* Static variables */
    char process_name;
    int burst_time;
    int arrival_time;
    /* Manipulated Variables */
    int time_in_waiting;
    int time_remaining;
    int completion_time;
    int completed;
    struct sim_pcb_node * meta_next;
    struct sim_pcb_node * ready_next;
    struct sim_pcb_node * ready_prev;
};
/* Meta Queue, the queue the simulation knows exists but the CPU doesn't*/
struct sim_pcb_node * meta_queue_head = NULL;
struct sim_pcb_node * meta_queue_tail = NULL;
/* Ready Queue */
struct sim_pcb_node * ready_queue_head = NULL;
struct sim_pcb_node * ready_queue_tail = NULL;
/* Function Declarations */
int read_in_file(char filename[]);
void refresh_start_state();
void update_ready_queue(int current_tick);
void write_summary_stats();
void increment_wait_times(struct sim_pcb_node *currently_executeing);
void reschedule_job(struct sim_pcb_node *job);
void sim_round_robin();
void sim_shortest_remaining_time_first();
void sim_shortest_time_first();
struct sim_pcb_node* find_shortest_time();

int read_in_file(char filename[]) 
{
    FILE * reading_file;
    int estimated_total_ticks = 0;
    if ((reading_file = fopen(filename, "r")) != NULL) 
    {
        while (!feof(reading_file)) {
            struct sim_pcb_node * new_node = malloc(sizeof(struct sim_pcb_node));
            fscanf(reading_file, "%c %d %d", 
                            &new_node->process_name,
                            &new_node->burst_time, 
                            &new_node->arrival_time);
            estimated_total_ticks += new_node->burst_time;
            new_node->time_remaining = new_node->burst_time;
            new_node->time_in_waiting = 0;
            new_node->meta_next = NULL;
            new_node->ready_next = NULL;
            new_node->completed = 0;
            if (new_node->burst_time != 0)
            {
                if (meta_queue_head == NULL)
                {
                    meta_queue_head = new_node;
                    meta_queue_tail = new_node;
                }
                else
                {
                    meta_queue_tail->meta_next = new_node;
                    meta_queue_tail = new_node;
                }
            }
        }
        fclose(reading_file);
    }
    return estimated_total_ticks;
}

void refresh_start_state()
{
    struct sim_pcb_node* current = meta_queue_head;
    ready_queue_head = NULL;
    ready_queue_tail = NULL;
    /* Reset all stats that are manipulated */
    while (current != NULL)
    {
        current->ready_next = NULL;
        current->ready_prev = NULL;
        current->time_in_waiting = 0;
        current->completion_time = 0;
        current->completed = 0;
        current->time_remaining = current->burst_time;
        
        current = current->meta_next;
    }
}

void update_ready_queue(int current_tick)
{
    /* Check Meta-Queue for currently arriveing jobs */
    struct sim_pcb_node *current;
    current = meta_queue_head;
    while (current != NULL)
    {
        if (current->time_remaining != 0)
        {
            if (current_tick == current->arrival_time)
            {
                if (ready_queue_head == NULL)
                {
                    current->ready_next = NULL;
                    current->ready_prev = NULL;
                    ready_queue_head = current;
                    ready_queue_tail = current;
                }
                else
                {
                    current->ready_prev = ready_queue_tail;
                    ready_queue_tail->ready_next = current;
                    ready_queue_tail = current;
                    current->ready_next = NULL;
                }
            }
        }
        current = current->meta_next;
    }
    /* Remove Completed Processes */
    current = ready_queue_head;
    while (current != NULL)
    {
        if (current->time_remaining == 0)
        {
            if (current == ready_queue_head)
            {
                ready_queue_head = current->ready_next;
                current->ready_next = NULL;
            }
            else if (current == ready_queue_tail)
            {
                ready_queue_tail = current->ready_prev;
                current->ready_prev->ready_next = NULL;
                current->ready_prev = NULL;
            }
            else
            {
                current->ready_prev->ready_next = current->ready_next;
                current->ready_next->ready_prev = current->ready_prev;
            }
        }

        current = current->ready_next;
    }
    // current = ready_queue_head;
    // while (current != NULL) {
    //     printf("%c->", current->process_name);
    //     current=current->ready_next;
    // }
    // printf("\n");
}

void write_summary_stats()
{
    /* Process ID | Turnaround Time | Waiting Time */
    /* Loop variable */
    struct sim_pcb_node *current;
    current = meta_queue_head;
    /* Running Totals */
    int total_turnaround_time = 0;
    float total_waiting_time = 0;
    float number_of_processes = 0;
    /* Draw chart */
    printf("%s", top_bar);
    printf("┃ Process\t┃ Turnaround\t┃ Waiting\t┃\n");
    printf("┃ Name\t\t┃ Time\t\t┃ Time\t\t┃\n");
    printf("%s", mid_bar);
    while (current != NULL )
    {
        /* Calculate Line Stat */
        int turnaround_time = current->completion_time - current->arrival_time;
        printf("┃ %c\t\t┃ %d\t\t┃ %d\t\t┃\n", 
            current->process_name, turnaround_time, current->time_in_waiting);
        /* Add to Totals */
        total_turnaround_time += turnaround_time;
        total_waiting_time += current->time_in_waiting;
        number_of_processes++;
        /* Increment Loop */
        current = current->meta_next;
    }
    /* Draw end of chart and average */
    printf("%s", mid_bar);
    printf("┃ Average\t┃ %d/%d=%.3f\t┃ %d/%d=%.3f\t┃\n", 
        (int) total_turnaround_time,
        (int) number_of_processes,
        total_turnaround_time/number_of_processes,
        (int) total_waiting_time,
        (int) number_of_processes,
        total_waiting_time/number_of_processes);
    printf("%s", bot_bar);

}

void increment_wait_times (struct sim_pcb_node *currently_executeing)
{
    struct sim_pcb_node *current = NULL;
    current = ready_queue_head;
    while (current != NULL)
    {
        if (currently_executeing != current)
        {
            current->time_in_waiting ++;
        }
        current = current->ready_next;
    }
}

void reschedule_job(struct sim_pcb_node *job)
{
    ready_queue_head = job->ready_next;
    ready_queue_tail->ready_next = job;
    ready_queue_tail = job;
    job->ready_next = NULL;
}

void sim_round_robin()
{
    /* Draw Header */
    printf("%s", header_top_bar);
    printf("%s Round Robin Simulation\t\t\t%s\n", vertical_bar, vertical_bar);
    printf("%s", header_bot_bar);
    /* Preserved Variables between Loops */
    int quantum = 3;        
    int current_tick = 0;   /* Global clock */
    int process_tick = 0;   /* Current executing processes used quantum */
    struct sim_pcb_node *re_job = NULL; /* Holder for a job */
    struct sim_pcb_node *currently_executeing = NULL; /* CPU Pointer */

    while (1)
    {
        /* Check if anything from the meta-queue should be added to the ready queue */
        update_ready_queue(current_tick);
        /*  Tick UP the quantum time, 
            tick DOWN the process completion */
        /* Check the Ready Queue */
        if (ready_queue_head == NULL)
        {
            currently_executeing->completion_time = current_tick;
            printf("  %d\tCompleted\n", current_tick);
            break;
        }
        /* No Executing Process */
        else if (currently_executeing == NULL)
        {
            /* Fetch the Head of the Ready Queue */
            currently_executeing = ready_queue_head;     
            printf("  %d %c", current_tick, currently_executeing->process_name);
            process_tick = 0;
        }
        /* Checking Process Burst Completion */
        else if (currently_executeing != NULL)
        {
            /* Check Job Completion */
            if (currently_executeing->time_remaining == 0)
            {
                printf("\tProcess Terminated.\n");
                currently_executeing->completion_time = current_tick;
                // printf("%c\t--Breaks Here--\n", currently_executeing->process_name);
                /* Fetch new process */
                if (currently_executeing->ready_next != NULL) {
                    currently_executeing = currently_executeing->ready_next;
                }
                else {
                    currently_executeing = ready_queue_head;
                }
                process_tick = 0;
                printf("  %d %c", current_tick, currently_executeing->process_name);
            }
            /* Checking Quantum Tick */
            else if (process_tick % quantum == 0)
            {
                printf("\tQuantum Expired - %dms Remaining.\n", 
                    currently_executeing->time_remaining);
                /* Fetch new process */
                if (currently_executeing->ready_next == NULL) {
                    /* No Other Jobs in Queue */
                }
                else {
                    re_job = currently_executeing;
                    reschedule_job(re_job);
                    currently_executeing = ready_queue_head;
                    printf("  %d %c", current_tick, currently_executeing->process_name);
                    process_tick = 0;
                }
            }
        }
        /* Update Waiting Stats except currently executing */
        increment_wait_times(currently_executeing);
        currently_executeing->time_remaining --;
        process_tick ++;
        current_tick ++;
    }

    write_summary_stats();
}

/* Search the ready queue for the shortest time on a job */
struct sim_pcb_node* find_shortest_time ()
{
    struct sim_pcb_node *current;
    struct sim_pcb_node *return_target;
    current = ready_queue_head;
    return_target = ready_queue_head;
    /* Linear Search run every call */
    while (current != NULL)
    {
        if ( current->time_remaining < return_target->time_remaining )
        {
            return_target = current;
        }
        current = current->ready_next;
    }
    
    return return_target;
}

void sim_shortest_remaining_time_first()
{
    /* Draw Header */
    printf("%s", header_top_bar);
    printf("┃ Shortest Remaining Time First Simulation \t┃\n");
    printf("%s", header_bot_bar);
    /* Set up unlooped variable */
    int current_tick = 0;                               /* global Clock */
    struct sim_pcb_node *current_shortest = NULL;       /* Used to hold the shortest job */
    struct sim_pcb_node *currently_executeing = NULL;   /* CPU Pointer */

    while (1)
    {
        /* Check if anything from the meta-queue should be added to the ready queue */
        update_ready_queue(current_tick);
        if(ready_queue_head == NULL)
        {
            /* Empty Queue means Completed 
                Probably an edge would cause this to fail early if there is no process
                in the ready queue */
            currently_executeing->completion_time = current_tick;
            printf("\tProcess Terminates.\n  %d\tCompleted\n", current_tick);
            break;
        }
        else if (currently_executeing == NULL)
        {
            /* Find the lowest time remaining */
            currently_executeing = find_shortest_time();
            printf("  %d %c", current_tick, currently_executeing->process_name);
        }
        else if (currently_executeing != NULL)
        {
            /* Check if current job is finished */
            if (currently_executeing->time_remaining == 0)
            {
                currently_executeing->completion_time = current_tick;
                printf("\tProcess Terminates.\n");
                /* Find the next job in queue */
                currently_executeing = find_shortest_time();
                printf("  %d %c", current_tick, currently_executeing->process_name);
            }
            /* Tick through check if a job needed to be preempted */
            else
            {
                current_shortest = find_shortest_time();
                /* Check if currently executing needs to be preempted */
                if (current_shortest != currently_executeing)
                {
                    printf("\tProcess Preempted - %dms left.\n", 
                        currently_executeing->time_remaining);
                    currently_executeing = current_shortest;
                    printf("  %d %c", current_tick, currently_executeing->process_name);
                }
            }
        }

        /* Update waiting queue stats */
        increment_wait_times(currently_executeing);
        /* Tick job down */
        currently_executeing->time_remaining --;
        current_tick ++;
    }
    /* Calculate and print the summary stats */
    write_summary_stats();
}

void sim_shortest_job_first()
{
    /* Draw Header */
    printf("%s", header_top_bar);
    printf("┃ Shortest Job First Simulation \t\t┃\n");
    printf("%s", header_bot_bar);
    /* Set up needed variables that are preseved between loops*/
    int current_tick = 0;
    struct sim_pcb_node *currently_executeing = NULL;

    while (1)
    {
        /* Check if anything from the meta-queue should be added to the ready queue */
        update_ready_queue(current_tick);

        if(ready_queue_head == NULL)
        {
            /* Empty Queue means Completed */
            currently_executeing->completion_time = current_tick;
            printf("\tProcess Terminates.\n  %d\tCompleted\n", current_tick);
            break;
        }
        else if (currently_executeing == NULL)
        {
            /* Find the lowest time remaining */
            currently_executeing = find_shortest_time();
            printf("  %d %c", current_tick, currently_executeing->process_name);
        }
        else if (currently_executeing != NULL)
        {
            /* Check if the current job is finished*/
            if (currently_executeing->time_remaining == 0)
            {
                currently_executeing->completion_time = current_tick;
                printf("\tProcess Terminates.\n");
                /* Find next Job */
                currently_executeing = find_shortest_time();
                printf("  %d %c", current_tick, currently_executeing->process_name);
            }
        }

        increment_wait_times(currently_executeing);
        currently_executeing->time_remaining --;
        current_tick ++;
    }

    write_summary_stats();
}

int main(int argc, char *argv[]) 
{
    /* Load File Contents into Memory */
    read_in_file(argv[1]);
    struct sim_pcb_node * current_position = meta_queue_head;
    /* Examine File Contents */
    printf("--------------- [ File Contents ] ---------------\n");
    printf("Process\t\tBurst Time\tArrival Time\n");
    while (current_position != NULL)
    {
        printf("%c\t\t%d\t\t%d\n",
            current_position->process_name, 
            current_position->burst_time,
            current_position->arrival_time);
        current_position = current_position->meta_next;
    }
    printf("--------------- [ File Contents ] ---------------\n");
    /* Round Robin Simulation */
    sim_round_robin();
    /* Refresh the meta queue to start state */
    refresh_start_state();
    /* SRTF Simulation */
    sim_shortest_remaining_time_first();
    /* Refresh the meta queue to start state */
    refresh_start_state();
    /* SJF Simulation */
    sim_shortest_job_first();

    return 0;
}