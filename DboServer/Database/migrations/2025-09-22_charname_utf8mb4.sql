-- Migrate character name columns to utf8mb4 to allow CJK names
-- Run this against the Characters DB

SET NAMES utf8mb4;
SET @db := DATABASE();

-- characters.CharName
ALTER TABLE `characters`
  MODIFY `CharName` VARCHAR(16) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  DROP INDEX `CharName` ,
  ADD UNIQUE KEY `ux_characters_charname` (`CharName`);

-- mail.FromName / TargetName (if present)
ALTER TABLE `mail`
  MODIFY `FromName` VARCHAR(32) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NULL,
  MODIFY `TargetName` VARCHAR(32) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NULL;

-- friendlist.friend_name
ALTER TABLE `friendlist`
  MODIFY `friend_name` VARCHAR(16) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NULL;

-- auctionhouse.Seller
ALTER TABLE `auctionhouse`
  MODIFY `Seller` VARCHAR(16) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NULL;

-- dojos.SeedCharName / LeaderName / GuildName
ALTER TABLE `dojos`
  MODIFY `SeedCharName` VARCHAR(16) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NULL,
  MODIFY `LeaderName` VARCHAR(16) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NULL,
  MODIFY `GuildName` VARCHAR(32) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NULL;

-- Optional: set database default charset (manual, uncomment if desired)
-- ALTER DATABASE `your_database_name` CHARACTER SET = utf8mb4 COLLATE = utf8mb4_unicode_ci;
