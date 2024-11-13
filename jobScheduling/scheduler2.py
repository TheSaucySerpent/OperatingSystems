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

  process_id = 0

  def __init__(self, arrival_time, num_cpu_bursts, cpu_bursts, io_bursts):
    self.id = Process.process_id
    self.arrival_time = arrival_time
    self.num_cpu_bursts = num_cpu_bursts
    self.cpu_bursts = cpu_bursts
    self.io_bursts = io_bursts
    
    self.state = Process.ProcessState.NEW # set initial process state to new

    self.start_time = None
    self.completion_time = None
    self.last_ready_time = arrival_time

    self.turn_around_time = 0
    self.wait_time = 0

    Process.process_id += 1 # increment the process id

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
    self.ready_queue = [] # round robin queue
    self.cpu_time = 0     # cpu time starts at 0
    self.cpu_idle = True  # track the status of the CPU
  
  def handle_arrival(self, event):
    event.process.state = Process.ProcessState.READY # set the process state to ready
    self.ready_queue.append(event.process)           # add the process to the ready queue
    event.process.last_ready_time = self.cpu_time
    self.print_process_state(event)                  # print the state of the process

    if self.cpu_idle:
        self.handle_running(event)
    
  def handle_running(self, event):
    self.cpu_idle = False                             # the cpu is now running
    next_process = self.ready_queue.pop(0)            # pop the next process in RR order  
    next_process.state = Process.ProcessState.RUNNING # set the process state to running
    self.print_process_state(event)                   # print the process state

    # if process is just now starting
    if next_process.start_time is None:
      next_process.start_time = self.cpu_time

    # update wait time
    next_process.wait_time += self.cpu_time - next_process.last_ready_time

    # update last ready time
    next_process.last_ready_time = self.cpu_time

    if next_process.cpu_bursts[0] <= self.quantum: # check if there will be io request, preemption, or termination
      self.handle_io_request(event)
    elif next_process.cpu_bursts[0] > self.quantum:
      self.handle_preemption(event)
    else:
      event.process.state = Process.ProcessState.EXIT
      self.handle_termination(event)

  def handle_preemption(self, event):
      self.cpu_idle = False
      self.cpu_time += self.quantum               # increment the cpu time by the quantum (since it used all of it)
      event.process.cpu_bursts[0] -= self.quantum # decrease the burst by quantum amount
      
      event.process_state = Process.ProcessState.BLOCKED
      self.print_process_state(event)

      # self.ready_queue.append(event.process)

      if event.process.io_bursts:
        io_request = Event(event.process, Event.EventType.IO_REQUEST, self.cpu_time)
        heapq.heappush(self.event_queue, io_request)

  def handle_io_request(self, event):
    self.cpu_idle = True
    self.cpu_time += event.process.cpu_bursts.pop(0) # increment the cpu time by duration of cpu_burst (since it didn't use all of it)
    
    if event.process.io_bursts:
      io_duration = event.process.io_bursts.pop(0)
      io_completion = Event(event.process, Event.EventType.IO_COMPLETION, self.cpu_time + io_duration)
      heapq.heappush(self.event_queue, io_completion)
      event.process.state = Process.ProcessState.BLOCKED
    else:
      self.handle_termination(event)

  def handle_io_completion(self, event):
    event.process.state = Process.ProcessState.READY
    event.process.last_ready_time = event.time
    self.ready_queue.append(event.process)
    self.cpu_idle = True
    self.print_process_state(event)
    self.handle_running(event)

  def handle_termination(self, event):
    event.process.completion_time = self.cpu_time
    event.process.turn_around_time = event.process.completion_time - event.process.arrival_time
    print(f'Job {event.process.id} terminated: Turn-Around-Time = {event.process.turn_around_time}, Wait time = {event.process.wait_time}')
      
  def print_process_state(self, event):
    print(f'Process {event.process.id} is in {event.process.state}')

  def run(self):
    while self.event_queue:
      event = heapq.heappop(self.event_queue) # pop the earliest event in the queue
      if self.cpu_time < event.process.arrival_time and event.event_type == Event.EventType.ARRIVAL:
        self.cpu_time = event.process.arrival_time
      
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
      