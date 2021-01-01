0
9
BEGIN L1
L56:
addi $t131, $zero, 1
add $t103, $t131, $zero
lw $t135, -4($fp)
lw $t134, -4($t135)
addi $t133, $t134, -4
lw $t132, 0($t133)
add $a0, $t132, $zero
jal ord
add $t125, $v0, $zero
add $t127, $t125, $zero
la $t136, L3
add $a0, $t136, $zero
jal ord
add $t126, $v0, $zero
slt $at, $t127, $t126
beq $at, $zero, L5
L6:
L9:
addi $t137, $zero, 0
add $t103, $t137, $zero
L8:
add $v0, $t103, $zero
j L55
L5:
lw $t141, -4($fp)
lw $t140, -4($t141)
addi $t139, $t140, -4
lw $t138, 0($t139)
add $a0, $t138, $zero
jal ord
add $t128, $v0, $zero
add $t130, $t128, $zero
la $t142, L4
add $a0, $t142, $zero
jal ord
add $t129, $v0, $zero
slt $at, $t129, $t130
beq $at, $zero, L8
L58:
L57:
L7:
j L9
L55:

END L1

 


BEGIN L2
L16:
lw $t150, -4($fp)
lw $t149, -4($t150)
addi $t148, $t149, -4
lw $t147, 0($t148)
add $a0, $t147, $zero
la $t151, L10
add $a1, $t151, $zero
jal strCmp
add $t143, $v0, $zero
addi $t152, $zero, 0
beq $t143, $t152, L12
L13:
lw $t156, -4($fp)
lw $t155, -4($t156)
addi $t154, $t155, -4
lw $t153, 0($t154)
add $a0, $t153, $zero
la $t157, L11
add $a1, $t157, $zero
jal strCmp
add $t144, $v0, $zero
addi $t158, $zero, 0
beq $t144, $t158, L17
L15:
addi $t159, $zero, 0
add $v0, $t159, $zero
j L59
L12:
L17:
lw $t162, -4($fp)
lw $t161, -4($t162)
addi $t160, $t161, -4
add $t146, $t160, $zero
jal getchar
add $t145, $v0, $zero
addi $t163, $zero, -2058347024
sw $t145, 0($t163)
j L16
L60:
L14:
j L17
L59:

END L2

0
BEGIN L0
L62:
addi $t173, $zero, 0
add $t101, $t173, $zero
add $a0, $fp, $zero
jal L2
addi $t174, $t100, 0
add $t172, $t174, $zero
add $a0, $fp, $zero
lw $t176, -4($fp)
lw $t175, -4($t176)
add $a1, $t175, $zero
jal L1
add $t171, $v0, $zero
addi $t177, $zero, -2057281664
sw $t171, 0($t177)
L20:
add $a0, $fp, $zero
lw $t179, -4($fp)
lw $t178, -4($t179)
add $a1, $t178, $zero
jal L1
add $t164, $v0, $zero
addi $t180, $zero, 0
bne $t164, $t180, L21
L18:
add $v0, $t101, $zero
j L61
L21:
addi $t182, $zero, 10
mult $t101, $t182
mflo $t181
add $t166, $t181, $zero
lw $t184, -4($fp)
lw $t183, -4($t184)
add $a0, $t183, $zero
jal ord
add $t165, $v0, $zero
add $t185, $t166, $t165
add $t168, $t185, $zero
la $t186, L19
add $a0, $t186, $zero
jal ord
add $t167, $v0, $zero
sub $t187, $t168, $t167
add $t101, $t187, $zero
lw $t189, -4($fp)
addi $t188, $t189, -4
add $t170, $t188, $zero
jal getchar
add $t169, $v0, $zero
addi $t190, $zero, -2057284800
sw $t169, 0($t190)
j L20
L61:

END L0

BEGIN L22
L64:
addi $t193, $zero, 4
add $a0, $t193, $zero
jal malloc
add $t109, $v0, $zero
add $t110, $t109, $zero
addi $t194, $zero, -2059354176
addi $t195, $zero, 0
sw $t195, 0($t194)
addi $t196, $t110, 4
add $t110, $t196, $zero
add $t108, $t109, $zero
lw $t197, -4($fp)
add $a0, $t197, $zero
add $a1, $t108, $zero
jal L0
add $t111, $v0, $zero
lw $t198, 0($t108)
addi $t199, $zero, 0
bne $t198, $t199, L26
L27:
addi $t200, $zero, 0
add $t114, $t200, $zero
L28:
add $v0, $t114, $zero
j L63
L26:
addi $t201, $zero, 8
add $a0, $t201, $zero
jal malloc
add $t112, $v0, $zero
add $t113, $t112, $zero
addi $t202, $zero, -2059350352
sw $t111, 0($t202)
addi $t203, $t113, 4
add $t113, $t203, $zero
add $t192, $t113, $zero
lw $t204, -4($fp)
add $a0, $t204, $zero
jal L22
add $t191, $v0, $zero
addi $t205, $zero, -2057257984
sw $t191, 0($t205)
addi $t206, $t113, 4
add $t113, $t206, $zero
add $t114, $t112, $zero
j L28
L63:

END L22

BEGIN L23
L66:
addi $t211, $zero, 0
beq $t104, $t211, L35
L36:
addi $t212, $zero, 0
beq $t105, $t212, L32
L33:
lw $t213, 0($t104)
lw $t214, 0($t105)
slt $at, $t213, $t214
bne $at, $zero, L29
L30:
addi $t215, $zero, 8
add $a0, $t215, $zero
jal malloc
add $t117, $v0, $zero
add $t118, $t117, $zero
addi $t216, $zero, -2059340000
lw $t217, 0($t105)
sw $t217, 0($t216)
addi $t218, $t118, 4
add $t118, $t218, $zero
add $t208, $t118, $zero
lw $t219, -4($fp)
add $a0, $t219, $zero
add $a1, $t104, $zero
lw $t220, 4($t105)
add $a2, $t220, $zero
jal L23
add $t207, $v0, $zero
addi $t221, $zero, -2057231136
sw $t207, 0($t221)
addi $t222, $t118, 4
add $t118, $t222, $zero
add $t119, $t117, $zero
L31:
add $t120, $t119, $zero
L34:
add $t121, $t120, $zero
L37:
add $v0, $t121, $zero
j L65
L35:
add $t121, $t105, $zero
j L37
L32:
add $t120, $t104, $zero
j L34
L29:
addi $t223, $zero, 8
add $a0, $t223, $zero
jal malloc
add $t115, $v0, $zero
add $t116, $t115, $zero
addi $t224, $zero, -2059343472
lw $t225, 0($t104)
sw $t225, 0($t224)
addi $t226, $t116, 4
add $t116, $t226, $zero
add $t210, $t116, $zero
lw $t227, -4($fp)
add $a0, $t227, $zero
lw $t228, 4($t104)
add $a1, $t228, $zero
add $a2, $t105, $zero
jal L23
add $t209, $v0, $zero
addi $t229, $zero, -2057225104
sw $t209, 0($t229)
addi $t230, $t116, 4
add $t116, $t230, $zero
add $t119, $t115, $zero
j L31
L65:

END L23

0
BEGIN L38
L68:
addi $t234, $zero, 0
slt $at, $t234, $t122
bne $at, $zero, L40
L41:
addi $t235, $zero, 0
add $v0, $t235, $zero
j L67
L40:
lw $t236, -4($fp)
add $a0, $t236, $zero
addi $t238, $zero, 10
div $t122, $t238
mflo $t237
add $a1, $t237, $zero
jal L38
addi $t242, $zero, 10
div $t122, $t242
mflo $t241
addi $t243, $zero, 10
mult $t241, $t243
mflo $t240
sub $t239, $t122, $t240
add $t233, $t239, $zero
la $t244, L39
add $a0, $t244, $zero
jal ord
add $t232, $v0, $zero
add $t245, $t233, $t232
add $a0, $t245, $zero
jal chr
add $t231, $v0, $zero
add $a0, $t231, $zero
jal print
j L41
L67:

END L38

-
0
BEGIN L24
L70:
addi $t246, $zero, 0
slt $at, $t106, $t246
bne $at, $zero, L47
L48:
addi $t247, $zero, 0
slt $at, $t247, $t106
bne $at, $zero, L44
L45:
la $t248, L43
add $a0, $t248, $zero
jal print
L46:
L49:
addi $t249, $zero, 0
add $v0, $t249, $zero
j L69
L47:
la $t250, L42
add $a0, $t250, $zero
jal print
add $a0, $fp, $zero
addi $t252, $zero, 0
sub $t251, $t252, $t106
add $a1, $t251, $zero
jal L38
j L49
L44:
add $a0, $fp, $zero
add $a1, $t106, $zero
jal L38
j L46
L69:

END L24



 
BEGIN L25
L72:
addi $t253, $zero, 0
beq $t107, $t253, L52
L53:
lw $t254, -4($fp)
add $a0, $t254, $zero
lw $t255, 0($t107)
add $a1, $t255, $zero
jal L24
la $t256, L51
add $a0, $t256, $zero
jal print
lw $t257, -4($fp)
add $a0, $t257, $zero
lw $t258, 4($t107)
add $a1, $t258, $zero
jal L25
L54:
addi $t259, $zero, 0
add $v0, $t259, $zero
j L71
L52:
la $t260, L50
add $a0, $t260, $zero
jal print
j L54
L71:

END L25

BEGIN main
L74:
addi $t267, $fp, -4
add $t264, $t267, $zero
jal getchar
add $t263, $v0, $zero
addi $t268, $zero, -2021639840
sw $t263, 0($t268)
add $a0, $fp, $zero
jal L22
add $t123, $v0, $zero
addi $t269, $fp, -4
add $t266, $t269, $zero
jal getchar
add $t265, $v0, $zero
addi $t270, $zero, -2021637536
sw $t265, 0($t270)
add $a0, $fp, $zero
jal L22
add $t124, $v0, $zero
add $t262, $fp, $zero
add $a0, $fp, $zero
add $a1, $t123, $zero
add $a2, $t124, $zero
jal L23
add $t261, $v0, $zero
add $a0, $t262, $zero
add $a1, $t261, $zero
jal L25
addi $t271, $zero, 0
add $v0, $t271, $zero
j L73
L73:

END main

