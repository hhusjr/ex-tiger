BEGIN L0
L2:
add $t110, $t100, $t101
add $t109, $t110, $t102
add $t108, $t109, $t103
add $t107, $t108, $t104
add $t106, $t107, $t105
add $v0, $t106, $zero
j L1
L1:

END L0

BEGIN main
L4:
add $a0, $fp, $zero
addi $t111, $zero, 1
add $a1, $t111, $zero
addi $t112, $zero, 2
add $a2, $t112, $zero
addi $t113, $zero, 3
add $a3, $t113, $zero
addiu $sp, $sp, -12
addi $t114, $zero, 6
sw $t114, 8($sp)
addi $t115, $zero, 5
sw $t115, 4($sp)
addi $t116, $zero, 4
sw $t116, 0($sp)
jal L0
add $v0, $v0, $zero
j L3
L3:

END main

