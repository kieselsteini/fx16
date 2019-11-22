""" FX16 assembler """
class Assembler(object):
    opcodes = {
        'nop': 0x00,
        'drop': 0x02, 'dup': 0x03, 'swap': 0x04, 'over': 0x05, 'rot': 0x06,
        'push': 0x07, 'pop': 0x08,
        'ldb': 0x09, 'ldw': 0x0a, 'stb': 0x0b, 'stw': 0x0c,
        'jmp': 0x0d, 'jz': 0x0e, 'jnz': 0x0f,
        'add': 0x10, 'sub': 0x11, 'mul': 0x12, 'div': 0x13, 'mod': 0x14,
        'and': 0x15, 'or': 0x16, 'xor': 0x17, 'not': 0x18,
        'shl': 0x19, 'shr': 0x1a,
        'eq': 0x1b, 'lt': 0x1c, 'le': 0x1d,
        'call': 0x1e, 'ret': 0x1f,
    }

    def __init__(self):
        self.labels = dict()
        self.references = list()
        self.memory = bytearray(0x10000)
        self.pc = 0

    def _poke8(self, ptr, value):
        self.memory[ptr % len(self.memory)] = value & 255

    def _poke16(self, ptr, value):
        self._poke8(ptr, value >> 8)
        self._poke8(ptr + 1, value)

    def _out8(self, value):
        self._poke8(self.pc, value)
        self.pc += 1

    def _out16(self, value):
        self._poke16(self.pc, value)
        self.pc += 2

    def _ref(self, label):
        self.references.append((label, self.pc))
        self._out16(0x0000)

    def _parse_int(self, str):
        try:
            if str[:2] == '0x' or str[:2] == '0X':
                return int(str[2:], 16)
            elif str[0] == '$':
                return int(str[1:], 16)
            elif str[-1] == 'h':
                return int(str[:-1], 16)
            else:
                return int(str)
        except ValueError:
            return None

    def _parse_command(self, command):
        # try to find opcode
        opcode = self.opcodes.get(command.lower())
        if opcode is not None:
            self._out16(opcode)
            return
        # try to convert to number
        number = self._parse_int(command)
        if number is not None:
            self._out16(0x01)
            self._out16(number)
            return
        # create a reference to a label
        self._ref(command)

    def _parse_line(self, line):
        for command in line.split():
            if command[0] == ';':  # comment
                return
            elif command[-1] == ':':  # label
                self.labels[command[:-1]] = self.pc
            elif command[0] == '@':  # push label reference
                self._out16(0x01)
                self._ref(command[1:])
            elif command[0] == '.':  # assemble byte
                self._out8(self._parse_int(command[1:]))
            elif command[0] == ',':  # assemble word
                self._out16(self._parse_int(command[1:]))
            elif command[0] == '>':  # set program counter
                self.pc = self._parse_int(command[1:])
            elif command[0] == '#':  # include file
                self.assemble(command[1:])
            else:
                self._parse_command(command)

    def assemble(self, filename):
        with open(filename, 'r') as file:
            for line in file:
                self._parse_line(line)

    def finalize(self, filename):
        # fix all references
        for label, position in self.references:
            target_position = self.labels.get(label)
            if target_position is None:
                raise Exception("undefined reference: '%s'" % label)
            self._poke16(position, target_position)
        # write binary
        with open(filename, 'wb') as file:
            file.write(self.memory)


if __name__ == '__main__':
    asm = Assembler()
    asm.assemble('demo.asm')
    asm.finalize('memory.bin')
