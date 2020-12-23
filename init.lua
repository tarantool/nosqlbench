local function get_env(key, t, default)
    assert(key, "key required")
    assert(t,   "type required")
    if default ~= nil and type(default) ~= t then
        error(("default is not of corresponding type for key='%s'"):format(
            key
        ))
    end

    local value = os.getenv(key)
    if value == nil then
        if default == nil then
            error(("env key='%s' was not specified"):format(key))
        end

        return default
    end

    if t == 'number' then
        return tonumber(value)
    elseif t == 'boolean' then
        return tonumber(value:upper()) == 'TRUE'
    else
        return value
    end
end

box.cfg {
    listen       = get_env('TT_LISTEN',       'number', 3301),
    memtx_memory = get_env('TT_MEMTX_MEMORY', 'number', 2000000000),
    wal_mode     = get_env('TT_WAL_MODE',     'string', 'none'),

    pid_file     = "./tarantool-server.pid",
    log          = "./tarantool-server.log",
    background   = true,
    checkpoint_interval = 0,
}

local idx_type = get_env('NOSQLBENCH_IDX_TYPE', 'string')

local s = box.schema.space.create('tester_nosqlbench')
s:create_index('primary', { type = idx_type, parts = { 1, 'unsigned' }})

box.once('access:guest', function()
    box.schema.user.grant('guest', 'create,read,write,execute', 'universe')
end)

-- vim:tw=4:sw=4:expandtab
