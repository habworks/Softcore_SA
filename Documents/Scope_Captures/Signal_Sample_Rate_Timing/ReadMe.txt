Sequence of events flows from top to bottom CH2 then CH1 then CH3 then CH4

CH 1 PL Timer 1 Interrupt goes active - it is cleared by the corresponding ISR
CH 2 PS Shows ISR response to PL Timer IRQ when it starts (high) and when it is completed (low) - part of what it does is start a single conversion cycle
CH 3 PL Shows the ADC 7476A x2 IRQ go active - it is cleared by the corresponding ISR
CH 4 PS Shows the ISR resonse to the PL ADC 7476A x2 IRQ when it starts (high) and when it is completed (low) - By the end of this you have started an ADC cycle and read the result

Cursor: The cursor shows how long the entire process takes and ultimately how fast a sample rate the current architecture can handle 74KHz but with some breathing room I use 50KHz
for the this architecture, but quite possible to use up to maybe 70KHz - but did not test.

the system architecture would have to change to get faster sample rate