#!/usr/bin/env ruby 
# Translated a JFlap binary automoaton into a nqc file for driving the uring machine
#

class State
  attr_reader :name, :id, :final, :initial
  attr_accessor :transitions

  def initialize(name, id, initial=false, final=false, transitions=[])
    @transitions = transitions
    @name = name
    @id = id
    @final = final
    @initial = initial
  end

  def deterministic_transitions?
    read = @transitions.map{|t| t.read }
    return (read.length == read.uniq.length)
  end
end


class Transition
  attr_accessor :from, :to, :read, :write, :move

  def initialize(from, to, read, write, move)
    @from = from
    @to = to
    @read = read
    @write = write
    raise "Movement for transitions must be (R)ight, (L)eft or (S)top." if not %w{L R S}.include?(move.upcase)
    @move = move.upcase
  end
end

class Automaton
  attr_reader :states, :transitions, :initial
  
  def initialize(filename)
    require 'rexml/document'
    require 'rexml/xpath'
    require 'rexml/attribute'
    parse_jflap(filename)
  end 

  # Is this automaton deterministic?
  def deterministic?
    @states.each{|s| return false if not s.deterministic_transitions? }
    return true
  end

  private

  def listify(doc, xpath, text=true)
    list = []
    REXML::XPath.each(doc, xpath){|x| list << ((text) ? x.text : x)}
    list
  end

  def parse_jflap(filename)
    # First, load file
    raise "Could not read input file" if not File.readable?(filename)
    doc = REXML::Document.new(File.read(filename))
    
    type = listify(doc, "/structure/type")[0]
    raise "That file is not a turing machine" if type != "turing"

    automata = listify(doc, "/structure/automaton", false)
    raise "No automata found." if automata.length == 0

    puts "Found #{automata.length} automata, loading first one." if automata.length > 1
    ######### End of Warnings ####################
   
    # Parse states 
    @states = {}
    listify(doc, "/structure/automaton[1]/block", false).each{|state|
      # Load final/initial states
      initial = listify(state, "initial").length > 0
      final   = listify(state, "final").length > 0

      # Load attributes
      id = listify(state, "@id", false)
      name = listify(state, "@name", false)
      raise "Not every state has a name or an ID.  Cannot parse!" if(id.length == 0 or name.length == 0)
      id = id[0].value 
      name = name[0].value

      # Spew to console for no good reason
      #puts "Found state:"
      #puts "\t initial: #{initial}"
      #puts "\t final: #{final}"
      #puts "\t GUID: #{id}"
      #puts "\t name: #{name}"
      #puts "\t #{state.inspect}"

      # Create state
      @states[id] = State.new(name, id, initial, final)
    }

    # Ensure there are not two start states
    @initial = false
    @states.each{|k,v|
      raise "Cannot have two initial states" if v.initial and @initial 
      @initial = v if v.initial
    }
    raise "No initial state detected" if not @initial

    # Parse transitions
    @transitions = []
    listify(doc, "/structure/automaton[1]/transition", false).each{|transition|
      # Load to, from, read, write, move values
      to      = listify(transition, "to")[0]
      from    = listify(transition, "from")[0]
      read    = listify(transition, "read")[0]
      write   = listify(transition, "write")[0]
      move    = listify(transition, "move")[0]


      # Add to list and register with states
      raise "One of your transitions" if not @states[from] or not @states[to]
      @transitions << Transition.new( @states[from], @states[to], read, write, move )
      @states[from].transitions << @transitions[@transitions.length-1]
    }
    #puts "Parsed successfully.  Found #{@states.keys.length} states and #{@transitions.length} transitions."
  end

end

class JFLapCompiler
  def initialize(automaton)
    @automaton = automaton
    compute_flags
  end

  def optimise
    # TODO
  end

HEADER = %{
#include "turing.c"

// Left, right, and a global to stop execution
#define R 0
#define L 1
#define S 2

int t_halt = 0;
int t_read = 0;
int t_write = 0;
int t_move = R;
}

LIBS = %{
// IMPORTANT: presumes the read value is in retval.
void turing(int wr, int mov)\{
  // record the read state.  Allows for optimisations
  t_read = retval;

  t_write = wr;
  t_move = mov;
\}

void error()\{
  t_halt = 1;
  PlaySound(SOUND_CLICK);
  PlaySound(SOUND_CLICK);
  PlaySound(SOUND_DOWN);
\}//end error
}

MAIN = %{
task main () \{
}

PULLUP = %{
  // Main drive motor and write head motor
  acquire(ACQUIRE_OUT_A + ACQUIRE_OUT_B)\{ 
  int stateval = 0;  // temporarily stores the state to avoid clobbering retval

  pullup(); // turing pullup routine
}

PULLDOWN = %{
  \} // end resource grab
}

ENDMAIN = %{
\} // end main
}

# ##################################################################
STARTWHILE = %{
  // Start at q0
  retval = %i;

      
  while(t_halt == 0)\{
        stateval = retval;  // copy state from retval
        read();             // Load read value into retval for the upcoming state to use
        switch(stateval)\{
}

CASES = %{
          case %i:
            s_%s();
            break;
}

ENDWHILE = %{
			\} // End case


      // Check for error
      if(retval == -1)
        error();

			// Optimisation to prevent writing when
			// already in correct state
			if(t_read != t_write)
				write(t_write);

			// Move.
			if(t_move == R)
				right();
			if(t_move == L)
				left();
    \} // end while trampoline
}

STARTSTATE = %{
void s_%s()\{
	//read();  now done in the while to save bytecode size
}

STARTCONDITON = %{
  switch(retval)\{
}

CONDITON = %{
  case %i:
    turing(%i, %s);
    retval = %i;
    break;
}

ENDCONDITON = %{
  default:
    retval = -1;
  \} // end case
}

ENDSTATE = %{
\} // end state %s
}
#else\{
#		turing(0, R);
#		retval = 1;
#	\}
#\}
#}


FINAL = %{
void s_%s()\{
  t_halt = 1;
  PlaySound(SOUND_CLICK);
  PlaySound(SOUND_CLICK);
  PlaySound(SOUND_UP);
\}
}


  def codegen(outfile=$stdout)
    outfile << HEADER
    outfile << LIBS
    
    @automaton.states.each{|id, state|
      if(state.final)
        outfile << FINAL % sanitise_name(state.name)
      else
        $stderr.puts "WARNING: State #{state.name} has no transitions.  Your automaton will _definitely_ error there." if not @flags[:nowarn] and state.transitions.length == 0
        outfile << STARTSTATE % sanitise_name(state.name)
        outfile << STARTCONDITON
        state.transitions.each{|transition|
          outfile << CONDITON % [transition.read.to_i, transition.write.to_i, transition.move.to_s.upcase, @flags[:state_nums][transition.to]]
        }
        outfile << ENDCONDITON 
        outfile << ENDSTATE % sanitise_name(state.name)
      end
    }

    outfile << MAIN
    outfile << PULLUP

    # Write start of the while loop
    outfile << STARTWHILE % @flags[:initial]
    @flags[:state_nums].each{|state, number|
      outfile << CASES % [number, sanitise_name(state.name)]
    }
    outfile << ENDWHILE 


    outfile << PULLDOWN
    outfile << ENDMAIN
  end

  private

  def sanitise_name(name)
    name.gsub(" ", "_")
  end
 
  def compute_flags
    @flags = {}

    # Number the states
    @flags[:state_nums] = {}
    count = 0
    @automaton.states.values.sort_by{|x| x.name}.each{|state|
      @flags[:state_nums][state] = count
      count += 1
    }

    @flags[:initial] = @flags[:state_nums][@automaton.initial]
  end

  def verify_automaton
    @automaton.transitions.each{|t|
      raise "This is a two-state turing machine, make your transitions switch on 0/1 only." if not ['0','1'].include?(t.read) or not ['0','1'].include?(t.read)
    }

    @automaton.states.each{|s|
      raise "The automaton provided is nondeterministic for state #{s.name}." if not s.deterministic_transitions?
    }
  end

  def verify_static_segments
    # TODO!
  end

end


infile = ARGV[0]
outfile = ((ARGV[1]) ? File.open(ARGV[1], 'w') : $stdout)
raise "Input document path must be provided" if not infile

automaton = Automaton.new(infile)
compiler = JFLapCompiler.new(automaton)
compiler.codegen(outfile)
