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
      if self.event_type in Event.priority_order and other.event_type in Event.priority_order:
        return Event.priority_order.index(self.event_type) < Event.priority_order.index(other.event_type)
      return self.process.id < other.process.id
    return self.time < other.time # for handling the earliest event first 

class RR_Scheduler:
  def __init__(self, quantum):
    self.quantum = quantum
    self.event_queue = [] # priority queue for events
    self.ready_queue = [] # round robin ready queue
    self.cpu_time = 0     # cpu time starts at 0
  
  def handle_arrival(self, event):
    self.print_process_state(event.process)           # print the state of the process (NEW)

    event.process.last_ready_time = event.time        # last time process was ready is when it arrived
    event.process.state = Process.ProcessState.READY  # set the process state to ready

    self.ready_queue.append(event.process)            # add the process to the ready queue
    self.print_process_state(event.process)           # print the state of the process (READY)
    
  def generate_event(self, process):
    process.state = Process.ProcessState.RUNNING # set the process state to running
    self.print_process_state(process)            # print the state of the process (RUNNING)

    # if this is the first time the process has been run, start time is now
    if process.start_time is None:
      process.start_time = self.cpu_time

    process.wait_time += self.cpu_time - process.last_ready_time # update wait time
    
    # preemption case
    if process.cpu_bursts[0] > self.quantum:
      event_time = self.cpu_time + self.quantum
      event = Event(process, Event.EventType.PREEMPTION, event_time)
      heapq.heappush(self.event_queue, event)
      
    else:
      # IO request case
      if process.io_bursts:
        event = Event(process, Event.EventType.IO_REQUEST, event_time)
        heapq.heappush(self.event_queue, event)
        
      # termination case
      else:
        process.state = Process.ProcessState.EXIT
        event = Event(process, Event.EventType.TERMINATION, process.cpu_bursts[0] + self.cpu_time)
        heapq.heappush(self.event_queue, event)

  def handle_preemption(self, event):
    event.process.cpu_bursts[0] -= self.quantum # decrease the burst by quantum amount

    event.process.state = Process.ProcessState.READY
    self.ready_queue.append(event.process)
    event.process.last_ready_time = self.cpu_time # update last ready time

    self.print_process_state(event.process)

  def handle_io_request(self, event):
    cpu_burst_duration = event.process.cpu_bursts.pop(0)

    event.process.state = Process.ProcessState.BLOCKED
    
    io_duration = event.process.io_bursts.pop(0)
    io_completion = Event(event.process, Event.EventType.IO_COMPLETION, self.cpu_time + io_duration)
    heapq.heappush(self.event_queue, io_completion)

    self.print_process_state(event.process)

  def handle_io_completion(self, event):
    event.process.state = Process.ProcessState.READY

    if event.process.cpu_bursts:
      self.ready_queue.append(event.process)
      self.print_process_state(event.process)
    else:  
      event.process.state = Process.ProcessState.EXIT
      event = Event(event.process, Event.EventType.TERMINATION, event.process.cpu_bursts[0] + self.cpu_time)
      heapq.heappush(self.event_queue, event)

  def handle_termination(self, event):
    event.process.completion_time = self.cpu_time
    event.process.turn_around_time = event.process.completion_time - event.process.arrival_time
    print(f'Job {event.process.id} terminated: Turn-Around-Time = {event.process.turn_around_time}, Wait time = {event.process.wait_time}')
      
  def print_process_state(self, process):
    print(f'CPU Time: {self.cpu_time} -- Process {process.id} is in process state {process.state.name}')

  def run(self):
    while self.event_queue or self.ready_queue:
      if self.event_queue:
        event = heapq.heappop(self.event_queue)
        self.cpu_time = event.time
        print(f"Processing event at time {self.cpu_time}: {event.event_type.name} for Process {event.process.id}")
        
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
            self.print_process_state(event.process)

      # run the next process in the ready queue
      elif self.ready_queue:
          current_process = self.ready_queue.pop(0)
          self.generate_event(current_process)

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
      