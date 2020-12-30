BEGIN main
L4:
addi $t103, $zero, 1
add $t100, $t103, $zero
addi $t104, $zero, 2
add $t101, $t104, $zero
beq $t100, $t101, L0
L1:
sub $t105, $t100, $t101
add $t102, $t105, $zero
L2:
add $v0, $t102, $zero
j L3
L0:
add $t106, $t100, $t101
add $t102, $t106, $zero
j L2
L3:

END main

