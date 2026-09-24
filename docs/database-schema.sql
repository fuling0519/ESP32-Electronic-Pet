-- 神秘蛋電子寵物資料庫 Schema
-- Database: SQLite 3
-- Date: 2026-09-24
-- Datetime values use ISO 8601 UTC text, for example: 2026-09-24T08:30:00Z.

PRAGMA foreign_keys = ON;

BEGIN TRANSACTION;

CREATE TABLE pet_species (
    species_id   TEXT PRIMARY KEY,
    species_name TEXT NOT NULL UNIQUE,
    asset_set_id TEXT NOT NULL UNIQUE,

    CHECK (length(trim(species_id)) > 0),
    CHECK (length(trim(species_name)) > 0),
    CHECK (length(trim(asset_set_id)) > 0)
);

CREATE TABLE pet (
    pet_id                     TEXT PRIMARY KEY,
    species_id                 TEXT,
    name                       TEXT NOT NULL,
    life_stage                 TEXT NOT NULL DEFAULT 'egg',
    satiety                    INTEGER NOT NULL DEFAULT 80,
    mood                       INTEGER NOT NULL DEFAULT 80,
    cleanliness                INTEGER NOT NULL DEFAULT 100,
    level                      INTEGER NOT NULL DEFAULT 1,
    exp                        INTEGER NOT NULL DEFAULT 0,
    is_sick                    INTEGER NOT NULL DEFAULT 0,
    is_dead                    INTEGER NOT NULL DEFAULT 0,
    age_seconds                INTEGER NOT NULL DEFAULT 0,
    born_at                    TEXT,
    starvation_awake_seconds   INTEGER NOT NULL DEFAULT 0,
    dirty_awake_seconds        INTEGER NOT NULL DEFAULT 0,
    sick_awake_seconds         INTEGER NOT NULL DEFAULT 0,
    sleep_mode                 TEXT NOT NULL DEFAULT 'awake',
    sleep_started_at           TEXT,
    last_updated_at            TEXT,
    time_base_id               TEXT,
    time_valid                 INTEGER NOT NULL DEFAULT 0,

    FOREIGN KEY (species_id)
        REFERENCES pet_species (species_id)
        ON UPDATE CASCADE
        ON DELETE RESTRICT,

    CHECK (length(trim(pet_id)) > 0),
    CHECK (length(name) BETWEEN 1 AND 8),
    CHECK (name NOT GLOB '*[^ -~]*'),
    CHECK (life_stage IN ('egg', 'hatched')),
    CHECK (
        (life_stage = 'egg' AND species_id IS NULL)
        OR (life_stage = 'hatched' AND species_id IS NOT NULL)
    ),
    CHECK (satiety BETWEEN 0 AND 100),
    CHECK (mood BETWEEN 0 AND 100),
    CHECK (cleanliness BETWEEN 0 AND 100),
    CHECK (level >= 1),
    CHECK (exp >= 0),
    CHECK (is_sick IN (0, 1)),
    CHECK (is_dead IN (0, 1)),
    CHECK (is_dead = 0 OR is_sick = 1),
    CHECK (age_seconds >= 0),
    CHECK (starvation_awake_seconds >= 0),
    CHECK (dirty_awake_seconds >= 0),
    CHECK (sick_awake_seconds >= 0),
    CHECK (sleep_mode IN ('awake', 'normal_sleep', 'deep_sleep')),
    CHECK (
        (sleep_mode = 'awake' AND sleep_started_at IS NULL)
        OR (sleep_mode IN ('normal_sleep', 'deep_sleep'))
    ),
    CHECK (time_valid IN (0, 1)),
    CHECK (time_base_id IS NULL OR length(trim(time_base_id)) > 0)
);

CREATE TABLE game (
    game_id   TEXT PRIMARY KEY,
    game_name TEXT NOT NULL UNIQUE,

    CHECK (length(trim(game_id)) > 0),
    CHECK (length(trim(game_name)) > 0)
);

CREATE TABLE play_session (
    session_id      TEXT PRIMARY KEY,
    pet_id          TEXT NOT NULL,
    game_id         TEXT NOT NULL,
    result          TEXT NOT NULL,
    score           INTEGER NOT NULL DEFAULT 0,
    started_at      TEXT,
    duration_seconds INTEGER NOT NULL DEFAULT 0,

    FOREIGN KEY (pet_id)
        REFERENCES pet (pet_id)
        ON UPDATE CASCADE
        ON DELETE RESTRICT,
    FOREIGN KEY (game_id)
        REFERENCES game (game_id)
        ON UPDATE CASCADE
        ON DELETE RESTRICT,

    CHECK (length(trim(session_id)) > 0),
    CHECK (result IN ('win', 'loss', 'draw', 'completed', 'aborted')),
    CHECK (score >= 0),
    CHECK (duration_seconds >= 0)
);

CREATE TABLE sleep_session (
    sleep_id       TEXT PRIMARY KEY,
    pet_id         TEXT NOT NULL,
    mode           TEXT NOT NULL,
    started_at     TEXT,
    ended_at       TEXT,
    elapsed_seconds INTEGER NOT NULL DEFAULT 0,
    wake_reason    TEXT,
    time_valid     INTEGER NOT NULL DEFAULT 0,

    FOREIGN KEY (pet_id)
        REFERENCES pet (pet_id)
        ON UPDATE CASCADE
        ON DELETE RESTRICT,

    CHECK (length(trim(sleep_id)) > 0),
    CHECK (mode IN ('normal_sleep', 'deep_sleep')),
    CHECK (elapsed_seconds >= 0),
    CHECK (
        wake_reason IS NULL
        OR wake_reason IN ('manual', 'button', 'timer', 'light', 'system')
    ),
    CHECK (time_valid IN (0, 1)),
    CHECK (time_valid = 0 OR (started_at IS NOT NULL AND ended_at IS NOT NULL)),
    CHECK (ended_at IS NULL OR started_at IS NULL OR ended_at >= started_at)
);

CREATE TABLE memorial (
    pet_id       TEXT PRIMARY KEY,
    name         TEXT NOT NULL,
    species_name TEXT NOT NULL,
    born_at      TEXT,
    died_at      TEXT,
    age_seconds  INTEGER NOT NULL,

    FOREIGN KEY (pet_id)
        REFERENCES pet (pet_id)
        ON UPDATE CASCADE
        ON DELETE RESTRICT,

    CHECK (length(name) BETWEEN 1 AND 8),
    CHECK (name NOT GLOB '*[^ -~]*'),
    CHECK (length(trim(species_name)) > 0),
    CHECK (age_seconds >= 0),
    CHECK (died_at IS NULL OR born_at IS NULL OR died_at >= born_at)
);

CREATE INDEX idx_pet_species_id
    ON pet (species_id);

CREATE INDEX idx_play_session_pet_started
    ON play_session (pet_id, started_at);

CREATE INDEX idx_play_session_game_score
    ON play_session (game_id, score DESC);

CREATE INDEX idx_sleep_session_pet_started
    ON sleep_session (pet_id, started_at);

-- 墓碑群最多保留 32 筆；滿額時須由玩家先明確刪除一筆。
CREATE TRIGGER memorial_limit_before_insert
BEFORE INSERT ON memorial
WHEN (SELECT COUNT(*) FROM memorial) >= 32
BEGIN
    SELECT RAISE(ABORT, 'memorial capacity reached; delete one memorial first');
END;

-- 只有已死亡的寵物才能建立墓碑。
CREATE TRIGGER memorial_dead_pet_before_insert
BEFORE INSERT ON memorial
WHEN NOT EXISTS (
    SELECT 1
    FROM pet
    WHERE pet.pet_id = NEW.pet_id
      AND pet.is_dead = 1
)
BEGIN
    SELECT RAISE(ABORT, 'memorial requires a dead pet');
END;

-- 墓碑建立後不可修改；玩家仍可依企劃明確刪除。
CREATE TRIGGER memorial_read_only_before_update
BEFORE UPDATE ON memorial
BEGIN
    SELECT RAISE(ABORT, 'memorial records are read-only');
END;

COMMIT;
