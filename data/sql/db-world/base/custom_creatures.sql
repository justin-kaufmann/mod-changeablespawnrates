-- Use world-database
USE acore_world;

-- Create a new table with the same structure and data (IMPORTANT: THIS IS ONLY A BACKUP-TABLE!)
CREATE TABLE custom_creatures AS
SELECT *
FROM creatures;