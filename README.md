Proyecto Final // Ensayos de un desvío
Antecedentes 
El presente proyecto consiste en el desarrollo de una obra de arte electrónico interactiva que se enmarca en la creación de un instrumento de luthería digital. Se trata de una escultura lumínica reactiva, construida con biomateriales que incorporan hilos de cobre en su tejido/estructura. Estos hilos, que sobresalen del cuerpo central de la pieza, funcionan como interfaces sensibles al contacto humano. Un microcontrolador ESP-32 se encarga de detectar cuándo los espectadores toman estos hilos, activando un sistema de iluminación LED integrado. El comportamiento de la luz es controlado por lógicas de condicionales e iteraciones: se ilumina completamente solo cuando los cinco hilos  entran en contacto con personas, presentando estados intermedios de luminosidad según la cantidad de interacciones y apagándose por completo en ausencia de contacto.

Una expansión significativa del proyecto incorpora una capa de memoria. El sistema registra y almacena en su memoria los distintos patrones de interacción que ha tenido con el público. Tras un período de inactividad, la obra entra en un estado de reminiscencia, reproduciendo automáticamente estas secuencias lumínicas grabadas, evocando así la historia de sus encuentros previos.

Este proyecto establece un diálogo con tres obras referentes del arte electrónico latinoamericano.

En primer lugar, dialoga con las obras de Yto Aranda. En particular con EXISTIR ES INTERVENIR
La autonomía es buena, en comunidad es mejor (2012). En donde la artista desarrolla una instalación, conformada de varios dispositivos interrelacionados entre sí. La misma se activa en su totalidad por medio de la participación de 8 espectadores y el concepto núcleo gira en torno a “nuestra existencia con lo otro / los otros, Yo soy, porque nosotros somos”.

En segundo lugar, el proyecto es dialoga con Mudo Temblor (2022) de Mariela Yeregui. De esta instalación robótica tomo la concepción de una corporalidad orgánica y vulnerable que reacciona ante la presencia del otro. Las "viscosidades hermanadas por el miedo" de Yeregui, que tiemblan ante la proximidad del espectador, encuentran un eco en la obra en cuestión. Ambas obras crean seres infotecnológicos con los que se cohabitan el espacio y cuya condición es profundamente relacional. Sin embargo, esta obra invierte la polaridad de la interacción: si en Yeregui la cercanía humana genera una reacción defensiva (temblor), en este caso la conexión, el contacto colectivo de los espectadores con la obra impacta en la iluminación de sí misma y en la construcción de su memoria. Proponiendo una narrativa de conexión como posibilidad de activación colectiva, sin dejar de lado, en su fase de "memoria", una reflexión sobre la huella que deja dicha interacción.

En tercer lugar, se relaciona con Pulse Room (2006) de Rafael Lozano-Hemmer. Al igual que en la instalación del artista mexicano, donde el ritmo cardíaco de los participantes se traduce en pulsos de luz que se almacenan y despliegan en una grilla, nuestra obra utiliza datos biométricos. En este caso, la contacto, para activar una respuesta lumínica la cual también será almacenada. Mientras Pulse Room archiva y exhibe el pulso individual en una cronología colectiva, este proyecto registra patrones de contacto, y de interacción grupal. Ambos algoritmos funcionan como estructuras centrales que traducen información corporal en un lenguaje lumínico, pero mientras Lozano-Hemmer se centra en la metáfora de la vida (el latido) internalizada en la máquina, esta obra explora la constitución de un estado colectivo y su posterior memoria.

La utilización de la conductividad de los biomateriales y del cuerpo humano como desencadenante del circuito evoca una larga tradición de obras que borran los límites entre lo orgánico y lo tecnológico. El proyecto se apropia de este principio pero lo orienta hacia una poética de la interconexión y la memoria, utilizando materiales producidos a partir de residuos orgánicos, un obra cuya gran parte es biodegradable, y otra porción que registra su historia de contactos.

En síntesis, articula la captura de datos corporales, la corporalidad orgánica y reactiva y la construcción de un relato a partir de la participación colectiva.  

Demostracion de video Demo.mp4
https://drive.google.com/file/d/1x3cjY_blpG9eejCWs9hJpyvKdoaSN2be/view?usp=sharing

Simulacro algoritmo:  https://wokwi.com/projects/445985431781028865



Pre Entrega // TRABAJO PRÁCTICO FINAL 

1. Un proyecto de obra (obra original completa o una etapa de la misma desarrollada a los fines del curso).
3. Un Instrumento (una creación de luthería digital, es decir un software que ofrezca apertura y versatilidad para su uso en diferentes obras, en vez de una creación ad hoc en función de una pieza en particular con una poética cerrada).


Sistema visual 
Uso de microcontrolador esp-32
Objeto de estudio central Biomateriales y su Conductividad. 
Condicionales e iteraciones

Forma de vida infotecnológica.
Especie de cúmulo hecho de biomaterial. El cual contiene en su estructura hilos de cobre. Los mismos sobresalen de la materia y son para interactuar con les espectadores. 
Supongamos que del centro circular de biomaterial salen hacia afuera 5 hilos de cobre. Los espectadores pueden tomar con sus manos cada hilo. El sistema de la obra sensa la presencia de cada participante que toma uno de los tentáculos. Cuando los 5 tentáculos son tomados por distintos participantes el centro de la obra se ilumina en su totalidad. Ahora cuando un participante suelta un tentáculo la región central que corresponde a ese tentáculo se apaga. La obra se va apagando a medida que los tentáculos quedan sin ser sujetos. Existe un estado de iluminación total (cuando todos los tentáculos son tomados por distintos participantes), un estado de apagón total (cuando todos los tentáculos son soltados por participantes) y estados intermedios en donde cada región se enciende según el tentáculo que está siendo tomado por el participante. 

Expansión: memoria de la obra - El sistema almacena las distintas variaciones de interacción que fue teniendo con el público y las guarda como patrones de iluminación. Cuando la obra queda en estado de apagón por determinada cantidad de minutos la obra comienza a recordar y a pasar cada una de las iteraciones desde x cantidad de horas. 

La obra aborda dos temas de estudio abordados en el temario. Por un lado los condicionales y por el otro las iteraciones.


# ESP32_Touch_LED

¿Qué hace el programa?
- Lee un sensor táctil (touchRead).
- Si el valor detectado es menor al umbral (30), enciende el LED.
- Si no, lo apaga.
- Imprime el valor leído en el Monitor Serial.


# ESP32_Touch_LED_Memoria

Cómo funciona
- Durante el contacto: el LED sube gradualmente hasta 255.
- Al soltar: baja gradualmente hasta 0.
- Archivado: mientras hay contacto, se muestrea el brillo cada 50 ms y se guarda como un “patrón”. Se guardan hasta 8 patrones, cada uno con hasta 800 muestras (≈40 s). Si se llena, el más antiguo se sobrescribe.
- Reproducción: si el LED estuvo apagado (brillo 0) por 1 minuto, comienza a reproducir los patrones guardados en bucle. Cada patrón termina con un fade out automático a 0.
- Interrupción: cualquier contacto corta de inmediato la reproducción y vuelve al control manual.
