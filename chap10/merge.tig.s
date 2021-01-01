0
9
BEGIN L1
L56:
addi $t131, $zero, 1
add $t103, $t131, $zero
lw $t134, -4($fp)
lw $t133, -4($t134)
lw $t132, -4($t133)
add $a0, $t132, $zero
jal ord
add $t125, $v0, $zero
add $t127, $t125, $zero
la $t135, L3
add $a0, $t135, $zero
jal ord
add $t126, $v0, $zero
slt $at, $t127, $t126
beq $at, $zero, L5
L6:
L9:
addi $t136, $zero, 0
add $t103, $t136, $zero
L8:
add $v0, $t103, $zero
j L55
L5:
lw $t139, -4($fp)
lw $t138, -4($t139)
lw $t137, -4($t138)
add $a0, $t137, $zero
jal ord
add $t128, $v0, $zero
add $t130, $t128, $zero
la $t140, L4
add $a0, $t140, $zero
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
lw $t147, -4($fp)
lw $t146, -4($t147)
lw $t145, -4($t146)
add $a0, $t145, $zero
la $t148, L10
add $a1, $t148, $zero
jal strCmp
add $t141, $v0, $zero
addi $t149, $zero, 0
beq $t141, $t149, L12
L13:
lw $t152, -4($fp)
lw $t151, -4($t152)
lw $t150, -4($t151)
add $a0, $t150, $zero
la $t153, L11
add $a1, $t153, $zero
jal strCmp
add $t142, $v0, $zero
addi $t154, $zero, 0
beq $t142, $t154, L17
L15:
addi $t155, $zero, 0
add $v0, $t155, $zero
j L59
L12:
L17:
lw $t158, -4($fp)
lw $t157, -4($t158)
addi $t156, $t157, -4
add $t144, $t156, $zero
jal getchar
add $t143, $v0, $zero
sw $t143, 0($t144)
j L16
L60:
L14:
j L17
L59:

END L2

0
BEGIN L0
L62:
addi $t168, $zero, 0
add $t101, $t168, $zero
add $a0, $fp, $zero
jal L2
addi $t169, $t100, 0
add $t167, $t169, $zero
add $a0, $fp, $zero
lw $t171, -4($fp)
lw $t170, -4($t171)
add $a1, $t170, $zero
jal L1
add $t166, $v0, $zero
sw $t166, 0($t167)
L20:
add $a0, $fp, $zero
lw $t173, -4($fp)
lw $t172, -4($t173)
add $a1, $t172, $zero
jal L1
add $t159, $v0, $zero
addi $t174, $zero, 0
bne $t159, $t174, L21
L18:
add $v0, $t101, $zero
j L61
L21:
addi $t176, $zero, 10
mult $t101, $t176
mflo $t175
add $t161, $t175, $zero
lw $t178, -4($fp)
lw $t177, -4($t178)
add $a0, $t177, $zero
jal ord
add $t160, $v0, $zero
add $t179, $t161, $t160
add $t163, $t179, $zero
la $t180, L19
add $a0, $t180, $zero
jal ord
add $t162, $v0, $zero
sub $t181, $t163, $t162
add $t101, $t181, $zero
lw $t183, -4($fp)
addi $t182, $t183, -4
add $t165, $t182, $zero
jal getchar
add $t164, $v0, $zero
sw $t164, 0($t165)
j L20
L61:

END L0

BEGIN L22
L64:
addi $t186, $zero, 4
add $a0, $t186, $zero
jal malloc
add $t109, $v0, $zero
add $t110, $t109, $zero
addi $t187, $zero, 0
sw $t187, 0($t110)
addi $t188, $t110, 4
add $t110, $t188, $zero
add $t108, $t109, $zero
lw $t189, -4($fp)
add $a0, $t189, $zero
add $a1, $t108, $zero
jal L0
add $t111, $v0, $zero
lw $t190, 0($t108)
addi $t191, $zero, 0
bne $t190, $t191, L26
L27:
addi $t192, $zero, 0
add $t114, $t192, $zero
L28:
add $v0, $t114, $zero
j L63
L26:
addi $t193, $zero, 8
add $a0, $t193, $zero
jal malloc
add $t112, $v0, $zero
add $t113, $t112, $zero
sw $t111, 0($t113)
addi $t194, $t113, 4
add $t113, $t194, $zero
add $t185, $t113, $zero
lw $t195, -4($fp)
add $a0, $t195, $zero
jal L22
add $t184, $v0, $zero
sw $t184, 0($t185)
addi $t196, $t113, 4
add $t113, $t196, $zero
add $t114, $t112, $zero
j L28
L63:

END L22

BEGIN L23
L66:
addi $t201, $zero, 0
beq $t104, $t201, L35
L36:
addi $t202, $zero, 0
beq $t105, $t202, L32
L33:
lw $t203, 0($t104)
lw $t204, 0($t105)
slt $at, $t203, $t204
bne $at, $zero, L29
L30:
addi $t205, $zero, 8
add $a0, $t205, $zero
jal malloc
add $t117, $v0, $zero
add $t118, $t117, $zero
lw $t206, 0($t105)
sw $t206, 0($t118)
addi $t207, $t118, 4
add $t118, $t207, $zero
add $t198, $t118, $zero
lw $t208, -4($fp)
add $a0, $t208, $zero
add $a1, $t104, $zero
lw $t209, 4($t105)
add $a2, $t209, $zero
jal L23
add $t197, $v0, $zero
sw $t197, 0($t198)
addi $t210, $t118, 4
add $t118, $t210, $zero
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
addi $t211, $zero, 8
add $a0, $t211, $zero
jal malloc
add $t115, $v0, $zero
add $t116, $t115, $zero
lw $t212, 0($t104)
sw $t212, 0($t116)
addi $t213, $t116, 4
add $t116, $t213, $zero
add $t200, $t116, $zero
lw $t214, -4($fp)
add $a0, $t214, $zero
lw $t215, 4($t104)
add $a1, $t215, $zero
add $a2, $t105, $zero
jal L23
add $t199, $v0, $zero
sw $t199, 0($t200)
addi $t216, $t116, 4
add $t116, $t216, $zero
add $t119, $t115, $zero
j L31
L65:

END L23

0
BEGIN L38
L68:
addi $t220, $zero, 0
slt $at, $t220, $t122
bne $at, $zero, L40
L41:
addi $t221, $zero, 0
add $v0, $t221, $zero
j L67
L40:
lw $t222, -4($fp)
add $a0, $t222, $zero
addi $t224, $zero, 10
div $t122, $t224
mflo $t223
add $a1, $t223, $zero
jal L38
addi $t228, $zero, 10
div $t122, $t228
mflo $t227
addi $t229, $zero, 10
mult $t227, $t229
mflo $t226
sub $t225, $t122, $t226
add $t219, $t225, $zero
la $t230, L39
add $a0, $t230, $zero
jal ord
add $t218, $v0, $zero
add $t231, $t219, $t218
add $a0, $t231, $zero
jal chr
add $t217, $v0, $zero
add $a0, $t217, $zero
jal print
j L41
L67:

END L38

-
0
BEGIN L24
L70:
addi $t232, $zero, 0
slt $at, $t106, $t232
bne $at, $zero, L47
L48:
addi $t233, $zero, 0
slt $at, $t233, $t106
bne $at, $zero, L44
L45:
la $t234, L43
add $a0, $t234, $zero
jal print
L46:
L49:
addi $t235, $zero, 0
add $v0, $t235, $zero
j L69
L47:
la $t236, L42
add $a0, $t236, $zero
jal print
add $a0, $fp, $zero
addi $t238, $zero, 0
sub $t237, $t238, $t106
add $a1, $t237, $zero
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
addi $t239, $zero, 0
beq $t107, $t239, L52
L53:
lw $t240, -4($fp)
add $a0, $t240, $zero
lw $t241, 0($t107)
add $a1, $t241, $zero
jal L24
la $t242, L51
add $a0, $t242, $zero
jal print
lw $t243, -4($fp)
add $a0, $t243, $zero
lw $t244, 4($t107)
add $a1, $t244, $zero
jal L25
L54:
addi $t245, $zero, 0
add $v0, $t245, $zero
j L71
L52:
la $t246, L50
add $a0, $t246, $zero
jal print
j L54
L71:

END L25

BEGIN main
L74:
addi $t253, $fp, -4
add $t250, $t253, $zero
jal getchar
add $t249, $v0, $zero
sw $t249, 0($t250)
add $a0, $fp, $zero
jal L22
add $t123, $v0, $zero
addi $t254, $fp, -4
add $t252, $t254, $zero
jal getchar
add $t251, $v0, $zero
sw $t251, 0($t252)
add $a0, $fp, $zero
jal L22
add $t124, $v0, $zero
add $t248, $fp, $zero
add $a0, $fp, $zero
add $a1, $t123, $zero
add $a2, $t124, $zero
jal L23
add $t247, $v0, $zero
add $a0, $t248, $zero
add $a1, $t247, $zero
jal L25
addi $t255, $zero, 0
add $v0, $t255, $zero
j L73
L73:

END main

