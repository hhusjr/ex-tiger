BEGIN main
L3:
addi $t105, $zero, 100
addi $t106, $zero, 4
mult $t105, $t106
mflo $t104
add $t101, $t104, $zero
add $t100, $t102, $zero
addi $t107, $zero, 1
add $t103, $t107, $zero
lw $t112, 8($t100)
addi $t111, $t112, 3
addi $t113, $zero, 4
mult $t111, $t113
mflo $t110
add $t109, $t100, $t110
lw $t108, 0($t109)
addi $t114, $zero, 4
beq $t108, $t114, L0
L1:
addi $t115, $zero, 0
add $t103, $t115, $zero
L0:
add $v0, $t103, $zero
j L2
L2:

END main

