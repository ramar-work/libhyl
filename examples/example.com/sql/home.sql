-- All menu items
-- Doing both is difficult
SELECT * FROM
	( SELECT * FROM category ) AS C
INNER JOIN
	( SELECT * FROM menu ) as M 
ON
	C.category = M.meal
ORDER BY
	C.category,
	M.item
DESC
LIMIT 10 

;
