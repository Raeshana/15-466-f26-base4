VAR evidence_num = 0
VAR anger_num = 0
VAR affection_num = 0

-> start

=== start ===

The interrogation room smells like potatoes.

Across from you sits Mr. Peeler, prime suspect in the murder of Mr. Potato Knishes.

You slide a photograph across the table.

"Recognize him?"

Mr. Peeler sighs.

"Of course I recognize him--he owed me twenty dollars."

"And where were you that night?"

"At home."

"Doing what?"

"Peeling."

+ [So you admit you peeled Mr. Potato Knishes?]
    ~ evidence_num += 1
    -> question

+ [How a-peeling.]
    ~ affection_num += 1
    -> flirt

+ [WHERE IS THE PROOF?!]
    ~ anger_num += 1
    -> pressure

===question===
You got the
GOOD ENDING
->END

===flirt===
You got the
YANDERE ENDING
->END

===pressure===
You got the
BAD ENDING
->END