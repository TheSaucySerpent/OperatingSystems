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
    self.turn_around_time = 0
    self.wait_time = 0

    Process.process_id += 1 # increment the process id

class Event():
  class EventType(Enum):
    ARRIVAL = 1
    PREMPTION = 2
    IO_REQUEST = 3
    IO_COMPLETION = 4
    TERMINATION = 5

  def __init__(self, process, event_type, time):
    self.process = process  # to whom this event applies
    self.time = time              # when the event will occur
    self.event_type = event_type  # type of event
  
  def __lt__(self, other):
    return self.time < other.time # for handling the earliest event first 

class RR_Scheduler:
  def __init__(self, quantum):
    self.quantum = quantum
    self.event_queue = [] # priority queue for events
    self.ready_queue = [] # round robin queue
  
  def handle_arrival(self, event):
        heapq.heappush(self.event_queue, event)

  def handle_preemption(self, event):
      # Logic for handling preemption
      pass

  def handle_io_request(self, event):
      # Logic for handling IO request
      pass

  def handle_io_completion(self, event):
      # Logic for handling IO completion
      pass

  def handle_termination(self, event):
      # Logic for handling termination
      pass

  def process_event(self, event):
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

  def run(self):
      while self.event_queue:
          event = heapq.heappop(self.event_queue)
          self.process_event(event)


def main():
  quantum = int(sys.argv[1]) # quantum is the second passed argument

  scheuler = RR_Scheduler(quantum)

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
      
      scheuler.handle_arrival(arrival_event)
  
  scheuler.run()
      