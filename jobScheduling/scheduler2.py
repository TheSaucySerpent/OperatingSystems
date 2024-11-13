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

  def __init__(self, process, event_type, time):
    self.process = process        # to whom this event applies
    self.time = time              # when the event will occur
    self.event_type = event_type  # type of event
  
  def __lt__(self, other):
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
    
  def dispatch_to_cpu(self, process):
    process.state = Process.ProcessState.RUNNING # set the process state to running
    self.print_process_state(process)            # print the state of the process (RUNNING)

    # if this is the first time the process has been run, start time is now
    if process.start_time is None:
      process.start_time = self.cpu_time

    process.wait_time += self.cpu_time - process.last_ready_time # update wait time
    # print('last ready time ', process.last_ready_time)
    # print('cpu_time', self.cpu_time)

    # preemption case
    if process.cpu_bursts[0] > self.quantum:
      self.cpu_time += self.quantum # increment the cpu time by the quantum (since it used all of it)
      process.cpu_bursts[0] -= self.quantum # decrease the burst by quantum amount

      process.state = Process.ProcessState.READY
      self.ready_queue.append(process)
      process.last_ready_time = self.cpu_time # update last ready time
      
      
      event_time = self.cpu_time + self.quantum
      event = Event(process, Event.EventType.PREEMPTION, event_time)

      self.print_process_state(process)              # print the process state
      heapq.heappush(self.event_queue, event)

    elif process.cpu_bursts[0] <= self.quantum:
      cpu_burst_duration = process.cpu_bursts.pop(0)
      self.cpu_time += cpu_burst_duration # increment the cpu time by cpu_burst (since it didn't use all of quantum)

      # IO request case
      if process.io_bursts:
        process.state = Process.ProcessState.BLOCKED
        event_time = self.cpu_time + cpu_burst_duration
        event = Event(process, Event.EventType.IO_REQUEST, event_time)

        self.print_process_state(process)              # print the process state
        heapq.heappush(self.event_queue, event)
      # termination case
      else:
        process.state = Process.ProcessState.EXIT
        self.print_process_state(process)

        process.completion_time = self.cpu_time
        process.turn_around_time = process.completion_time - process.arrival_time

        print(f'Job {process.id} terminated: Turn-Around-Time = {process.turn_around_time}, Wait time = {process.wait_time}')

  def handle_preemption(self, event):
    event.process.state = Process.ProcessState.READY
    self.ready_queue.append(event.process)      # add the process back to the ready queue

  def handle_io_request(self, event):
    event.process.state = Process.ProcessState.BLOCKED
    self.print_process_state(event.process)
    
    io_duration = event.process.io_bursts.pop(0)
    io_completion = Event(event.process, Event.EventType.IO_COMPLETION, self.cpu_time + io_duration)
    heapq.heappush(self.event_queue, io_completion)

  def handle_io_completion(self, event):
    event.process.state = Process.ProcessState.READY

    if event.process.cpu_bursts:
      self.ready_queue.append(event.process)
      self.print_process_state(event)
    else:  
      event.process.state = Process.ProcessState.EXIT
      self.print_process_state(event.process)

      event.process.completion_time = self.cpu_time
      event.process.turn_around_time = event.process.completion_time - event.process.arrival_time

      print(f'Job {event.process.id} terminated: Turn-Around-Time = {event.process.turn_around_time}, Wait time = {event.process.wait_time}')
      
  def print_process_state(self, process):
    print(f'Process {process.id} is in {process.state}')

  def run(self):
    while self.event_queue or self.ready_queue:
        if self.ready_queue:
            current_process = self.ready_queue.pop(0)
            self.dispatch_to_cpu(current_process)
        elif self.event_queue:
            next_event_time = self.event_queue[0].time
            if next_event_time > self.cpu_time:
                self.cpu_time = next_event_time

        while self.event_queue and self.event_queue[0].time <= self.cpu_time:
            event = heapq.heappop(self.event_queue)
            if event.event_type == Event.EventType.ARRIVAL:
                self.handle_arrival(event)
            elif event.event_type == Event.EventType.PREEMPTION:
                self.handle_preemption(event)
            elif event.event_type == Event.EventType.IO_REQUEST:
                self.handle_io_request(event)
            elif event.event_type == Event.EventType.IO_COMPLETION:
                self.handle_io_completion(event)
            elif event.event_type == Event.EventType.TERMINATION:
                self.print_process_state(event.process)

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
      