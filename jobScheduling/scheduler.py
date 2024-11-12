import sys
import heapq
from enum import Enum

class EventType(Enum):
  CPU_BURST = 0
  IO_BURST = 1

class ProcessState(Enum):
  NEW = 1
  READY = 2
  RUNNING = 3
  BLOCKED = 4
  EXIT = 5

class Event:
  def __init__(self, process_id, event_type, time):
    self.process_id = process_id  # to whom this event applies
    self.time = time              # when the event will occur
    self.event_type = event_type  # type of event
  
  def __lt__(self, other):
    return self.time < other.time

class Process:
  process_id = 0
  def __init__(self, arrival_time, num_cpu_bursts, cpu_bursts, io_bursts):
    self.id = Process.process_id
    self.arrival_time = arrival_time
    self.num_cpu_bursts = num_cpu_bursts
    self.cpu_bursts = cpu_bursts
    self.io_bursts = io_bursts
    
    self.priority = self.arrival_time # set initial priority to arrival time
    self.event_type = EventType.CPU_BURST # set initial event type to CPU_BURST
    self.state = ProcessState.NEW # set initial process state to new
    self.turn_around_time = 0
    self.wait_time = 0

    Process.process_id += 1 # increment the process id
  
  
  def __lt__(self, other):
    return self.arrival_time < other.arrival_time
  

  def perform_cpu_burst(self, quantum):
    time_remaining = self.cpu_bursts.pop(0) - quantum

    if time_remaining > 0:
      self.cpu_bursts.insert(0, time_remaining)
  
    return time_remaining

  def perform_io_burst(self):
    return self.io_bursts.pop(0)



def main():
  quantum = int(sys.argv[1]) # quantum is the second passed argument
  prioirty_queue = []        # priority queue for events
  ready_queue = []           # queue of cars waiting at traffic light

  # open the text file
  with open('input_file.txt', 'r') as file:
    for line in file:
      values = line.split()

      arrival_time = int(values[0])   # first number is the arrival time to the CPU
      num_cpu_bursts = int(values[1]) # second number is the number of CPU bursts

      cpu_bursts = [int(x) for x in values[2::2]] # list slicing for cpu_bursts
      io_bursts = [int(x) for x in values[3::2]]  # list slicing for io_bursts

      process = Process(arrival_time, num_cpu_bursts, cpu_bursts, io_bursts) # create a new process
      ready_queue.append(process)                                            # append the process to the ready queue
      
  cpu_time = 0          # cpu time starts at 0
  first_process = True  # this is the first process being handled
  
  # loop processes are ready or there are events in priority queue
  while ready_queue or prioirty_queue:
    # if there are ready processes
    if ready_queue and (first_process or cpu_time >= ready_queue[0].arrival_time):
      process = ready_queue.pop(0)        # take the first process in the ready queue
      if first_process:
        cpu_time = process.arrival_time   # nothing happens until the first process arrives, so cpu time is the ready time of the first process
        first_process = False             # signal that it is no longer the first process

      
      process.state = ProcessState.READY  # set the process in the ready state
      print('Process', process.id, 'is in ready state')
      event = Event(process.id, process.event_type, cpu_time)
      heapq.heappush(prioirty_queue, event)
    
    if prioirty_queue:
      event = heapq.heappop(prioirty_queue)
      process_id = event.process_id
      event_type = event.event_type
      print(f"Handling event for process {process_id}, event type: {event_type}")

      # # find the process
      # process = next((p for p in ready_queue if p.id == process_id), None)
      # if not process:
      #   print('no')
      #   continue

      # handle CPU Burst
      if event_type == EventType.CPU_BURST and process.cpu_bursts:
        time_remaining = process.perform_cpu_burst(quantum)
        cpu_time += quantum
    
        if time_remaining < 0:
          cpu_time += time_remaining # time remaining will be negative
          process.state = ProcessState.BLOCKED # move to blocked state
          print('Process', process.id, 'is in blocked state')
          
        # there are still cpu_bursts that need to be taken care of
        if process.cpu_bursts:
          ready_queue.append(process)   
      
      # handle io burst
      elif event_type == EventType.IO_BURST and process.io_bursts:
        cpu_time = process.perform_io_burst()
        if process.cpu_bursts:
          process.state = ProcessState.READY
          process.event_type = EventType.CPU_BURST 
          ready_queue.append(process)
        
      if not process.cpu_bursts and not process.io_bursts:
        process.state = ProcessState.EXIT
        print(f"Process {process.id} completed.")


if __name__ == '__main__':
  main()