from enum import Enum
import heapq
import sys

class Process:
  class ProcessState(Enum):
    NEW = 1
    READY = 2
    RUNNING = 3
    BLOCKED = 4
    EXIT = 5

  process_id = 1

  def __init__(self, arrival_time, num_cpu_bursts, cpu_bursts, io_bursts):
    self.id = Process.process_id
    self.arrival_time = arrival_time
    self.num_cpu_bursts = num_cpu_bursts
    self.cpu_bursts = cpu_bursts
    self.io_bursts = io_bursts
    
    self.state = Process.ProcessState.NEW # set initial process state to new

    self.start_time = None                # process hasn't started yet
    self.completion_time = None           # process hasn't completed yet
    self.last_ready_time = None           # going to set last ready time in arrival

    self.turn_around_time = 0             # process hasn't completed or started, so turn around time unknown
    self.wait_time = 0                    # process hasn't had to wait yet

    Process.process_id += 1               # increment the process id

class Event():
  class EventType(Enum):
    ARRIVAL = 1
    PREEMPTION = 2
    IO_REQUEST = 3
    IO_COMPLETION = 4
    TERMINATION = 5

  priority_order = [EventType.ARRIVAL, EventType.IO_COMPLETION, EventType.PREEMPTION]

  def __init__(self, process, event_type, time):
    self.process = process        # to whom this event applies
    self.time = time              # when the event will occur
    self.event_type = event_type  # type of event
  
  def __lt__(self, other):
    if self.time == other.time:
      if self.event_type == other.event_type:
        return self.process.id < other.process.id
      
      elif self.event_type in Event.priority_order and other.event_type in Event.priority_order:
        return Event.priority_order.index(self.event_type) < Event.priority_order.index(other.event_type)
      
      return self.process.id < other.process.id
    
    return self.time < other.time # for handling the earliest event first 

class RR_Scheduler:
  def __init__(self, quantum):
    self.quantum = quantum
    self.event_queue = [] # priority queue for events
    self.ready_queue = [] # round robin ready queue
    self.cpu_time = 0     # cpu time starts at 0
    self.total_cpu_active_time = 0 # track time cpu was active for cpu usage
    self.completed_processes = [] # a list of the processes that have finished
  
  def generate_event(self, process):
    if process.start_time is None:
      process.start_time = self.cpu_time         # if this is the first time the process has been run, start time is now

    process.state = Process.ProcessState.RUNNING # set the process state to running
    self.print_process_state(process)            # print the state of the process (RUNNING)
    
    future_time = self.cpu_time                  # calculate when the next event can occur
    if self.event_queue:
      future_cpu_times = [event.time for event in self.event_queue if event.event_type in [Event.EventType.PREEMPTION, Event.EventType.IO_REQUEST, Event.EventType.TERMINATION]]
      if future_cpu_times:
        future_time = max(future_time, max(future_cpu_times))

    # preemption case
    if process.cpu_bursts[0] > self.quantum:
      event_time = future_time + self.quantum                           # premption time = time + quantum
      event = Event(process, Event.EventType.PREEMPTION, event_time)    # create the premption event
      heapq.heappush(self.event_queue, event)                           # add the preemption event to the event queue

    # the burst is less than or equal to the quantum (either doing IO burst or terminating)
    else:
      # IO request case
      if process.io_bursts:
        event_time = future_time + process.cpu_bursts[0]                # IO request time = time + burst duration
        event = Event(process, Event.EventType.IO_REQUEST, event_time)  # create the IO request event
        heapq.heappush(self.event_queue, event)                         # add the IO request event to the event queue

      # termination case
      else:
        event_time = future_time + process.cpu_bursts[0]                # termination time = time + burst duration
        event = Event(process, Event.EventType.TERMINATION, event_time) # create the termination event
        heapq.heappush(self.event_queue, event)                         # add the termination event to the event queue

  def handle_arrival(self, event):
    self.print_process_state(event.process)           # print the state of the process (NEW)

    event.process.state = Process.ProcessState.READY  # set the process state to ready
    event.process.last_ready_time = event.time        # last time process was ready is when it arrived

    self.ready_queue.append(event.process)            # add the process to the ready queue
    self.print_process_state(event.process)           # print the state of the process (READY)
    
  def handle_preemption(self, event):
    event.process.cpu_bursts[0] -= self.quantum       # decrease the burst by quantum amount

    event.process.state = Process.ProcessState.READY  # set the process state to ready
    event.process.last_ready_time = self.cpu_time     # update last ready time

    self.ready_queue.append(event.process)            # add the process to the ready queue
    self.print_process_state(event.process)           # print the state of the process (READY)

  def handle_io_request(self, event):
    event.process.wait_time += self.cpu_time - event.process.last_ready_time        # update wait time

    event.process.state = Process.ProcessState.BLOCKED                              # set the process state to blocked
    self.print_process_state(event.process)                                         # print the state of the process (BLOCKED)
    
    io_completion_time = self.cpu_time + event.process.io_bursts.pop(0)
    event = Event(event.process, Event.EventType.IO_COMPLETION, io_completion_time) # create IO completion event
    heapq.heappush(self.event_queue, event)                                         # add the IO completion to the event queue

  def handle_io_completion(self, event):
    event.process.state = Process.ProcessState.READY  # set the process state to ready
    event.process.last_ready_time = self.cpu_time     # update last ready time

    self.ready_queue.append(event.process)            # add the process to the ready queue
    self.print_process_state(event.process)           # print the state of the process (READY)

  def handle_termination(self, event):
    # set the completion time to the current CPU time
    event.process.completion_time = self.cpu_time
    event.process.turn_around_time = event.process.completion_time - event.process.arrival_time # calculate turn around time

    # print the termination messaage
    print(f'Process {event.process.id} terminated: Turn-Around-Time = {event.process.turn_around_time}, Wait time = {event.process.wait_time}')
    self.completed_processes.append(event.process) # add the process to the completed processes list
      
  def print_process_state(self, process):
    print(f'CPU Time: {self.cpu_time} -- Process {process.id} is in process state {process.state.name}')
  
  def output_summary_stats(self):
    total_turnaround_time = sum(p.turn_around_time for p in self.completed_processes)
    total_wait_time = sum(p.wait_time for p in self.completed_processes)
    num_processes = len(self.completed_processes)
    total_simulation_time = self.cpu_time

    if num_processes > 0:
        avg_turnaround_time = total_turnaround_time / num_processes
        avg_wait_time = total_wait_time / num_processes
        cpu_utilization = (self.total_cpu_active_time / total_simulation_time) * 100  # Percentage

        print("\n--- Simulation Summary ---")
        print(f"CPU Utilization: {cpu_utilization:.2f}%")
        print(f"Average Turnaround Time: {avg_turnaround_time:.2f}")
        print(f"Average Wait Time: {avg_wait_time:.2f}")
    else:
        print("No processes were completed in the simulation.")

  def run(self):
    while self.event_queue or self.ready_queue:
      if self.event_queue:
        event = heapq.heappop(self.event_queue)
        self.cpu_time = event.time
        print(f"CPU Time: {self.cpu_time} -- {event.event_type.name} for Process {event.process.id}")
        
        if event.event_type == Event.EventType.ARRIVAL:
            self.handle_arrival(event)
        elif event.event_type == Event.EventType.PREEMPTION:
            self.handle_preemption(event)
        elif event.event_type == Event.EventType.IO_REQUEST:
            self.handle_io_request(event)
        elif event.event_type == Event.EventType.IO_COMPLETION:
            self.handle_io_completion(event)
        elif event.event_type == Event.EventType.TERMINATION:
            self.handle_termination(event)

      # run the next process in the ready queue
      if self.ready_queue:
          current_process = self.ready_queue.pop(0)
          self.generate_event(current_process)
    
    self.output_summary_stats()

def main():
  quantum = int(sys.argv[1]) # quantum is the second passed argument

  schduler = RR_Scheduler(quantum)

  # open the text file
  with open('input_file.txt', 'r') as file:
    for line in file:
      values = line.split()

      arrival_time = int(values[0])   # first number is the arrival time to the CPU
      num_cpu_bursts = int(values[1]) # second number is the number of CPU bursts

      cpu_bursts = [int(x) for x in values[2::2]] # list slicing for cpu_bursts
      io_bursts = [int(x) for x in values[3::2]]  # list slicing for io_bursts

      process = Process(arrival_time, num_cpu_bursts, cpu_bursts, io_bursts) # create a new process

      arrival_event = Event(process, Event.EventType.ARRIVAL, arrival_time)
      
      heapq.heappush(schduler.event_queue, arrival_event)
  
  schduler.run()

if __name__ == '__main__':
   main()
      