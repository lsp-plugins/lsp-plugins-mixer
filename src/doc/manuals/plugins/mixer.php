<?php
	plugin_header();
	$m      =   ($PAGE == 'mixer_mono') ? 'm' : 's';
	$nc     =   (strpos($PAGE, '_x16') > 0) ? '16' : (
		        (strpos($PAGE, '_x8') > 0) ? '8' : '4');
?>

<p>
	This plugin performs mixing of <?= $nc ?> <?= ($m == 'm') ? 'mono' : 'stereo' ?> channels.
</p>
<p>
	It allows to mix the channels and apply the result mix to the additional master input.
	All the mixed result signal is passed to the and master output. 
</p>

<p><b>Main controls:</b></p>
<ul>
	<li>
		<b>Bypass</b> - bypass switch, when turned on (led indicator is shining), the output signal is similar to input signal. That does not mean
		that the plugin is not working.
	</li>
	<li><b>Dry amount</b> - the amount of the unprocessed (dry) signal in the output signal.</li>
	<li><b>Wet amount</b> - the amount of the processed (wet) signal in the output signal.</li>
	<?php if ($m == 's') { ?>
    <li><b>Balance</b> - the balance between left and right output channels in the mix.</li>
    <li><b>Mono</b> - the button that allows to switch the output signal to mono for mono compatibility test purpose.</li>
    <?php } ?>
    <li><b>In</b> - the level of the master input signal.</li>
    <li><b>Out</b> - the level of the master output signal.</li>
</ul>
<p><b>Mixer channel controls:</b></p>
<ul>
	<li><b>S</b> - solo the channel.</li>
	<li><b>M</b> - mute the channel.</li>
	<li><b>P</b> - invert the phase for the channel.</li>
	<?php if ($m == 's') { ?>
	<li><b>Pan</b> - the panning knobs for both left and right channels of the stereo channel.</li>
	<li><b>Balance</b> - the balance between left and right channels of the stereo channel.</li>
	<?php } ?>
	<li><b>Fader</b> - the overall output gain adjustment for the channel.</li>
	<li><b>Meter</b> - the overall output gain meter for the channel.</li>
</ul>
