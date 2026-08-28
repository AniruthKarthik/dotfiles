local M = {}
local state = {
  win = nil,
  buf = nil,
  job_id = nil,
  visible = false,
}

local function quit_process()
  if state.job_id and vim.fn.jobwait({ state.job_id }, 0)[1] == -1 then
    vim.fn.jobstop(state.job_id)
  end
  if state.win and vim.api.nvim_win_is_valid(state.win) then
    vim.api.nvim_win_close(state.win, true)
  end
  state = { win = nil, buf = nil, job_id = nil, visible = false }
end

local function toggle_window()
  if state.visible then
    if state.win and vim.api.nvim_win_is_valid(state.win) then
      vim.api.nvim_win_hide(state.win)
    end
    state.visible = false
  elseif state.buf and vim.api.nvim_buf_is_valid(state.buf) then
    local width = math.floor(vim.o.columns * 0.8)
    local height = math.min(20, vim.o.lines - 4)
    local row = math.floor((vim.o.lines - height) / 2)
    local col = math.floor((vim.o.columns - width) / 2)
    state.win = vim.api.nvim_open_win(state.buf, true, {
      relative = "editor",
      width = width,
      height = height,
      row = row,
      col = col,
      style = "minimal",
      border = "rounded",
      focusable = true,
    })
    state.visible = true
  else
    vim.notify("No active output buffer to show", vim.log.levels.WARN)
  end
end

function M.compile_and_run()
  local ext = vim.fn.expand("%:e")
  local filetype = vim.bo.filetype
  local file = vim.fn.expand("%:p")
  local name = vim.fn.expand("%:t:r")

  if vim.bo.modified then vim.cmd("write") end

  if ext == "c" or ext == "cpp" then
    local compile_cmd = ext == "c"
      and string.format("gcc '%s' -o /tmp/%s && /tmp/%s", file, name, name)
      or string.format("g++ '%s' -o /tmp/%s && /tmp/%s", file, name, name)
    vim.cmd("split")
    vim.cmd("terminal " .. compile_cmd)
    vim.cmd("startinsert")
    return
  end

  local cmd = ({
    py = "python3 " .. vim.fn.shellescape(file),
    java = string.format("javac %s && java %s", file, name),
    rs = string.format("rustc %s && ./%s", file, name),
    sh = "bash " .. vim.fn.shellescape(file),
    lua = "lua " .. vim.fn.shellescape(file),
  })[ext]

  if not cmd and (filetype == "audio" or filetype == "video" or ext == "mp4") then
    vim.fn.jobstart("mpv " .. vim.fn.shellescape(file), { detach = true })
    return
  elseif not cmd and (filetype == "png" or filetype == "jpg" or filetype == "jpeg") then
    vim.fn.jobstart("mpv " .. vim.fn.shellescape(file) .. " --keep-open --ontop", { detach = true })
    return
  elseif not cmd then
    vim.notify("Unsupported filetype: " .. ext .. " (" .. filetype .. ")", vim.log.levels.WARN)
    return
  end

  if not (state.buf and vim.api.nvim_buf_is_valid(state.buf)) then
    state.buf = vim.api.nvim_create_buf(false, true)
  end
  local buf = state.buf
  vim.api.nvim_buf_set_option(buf, "filetype", "output")
  vim.api.nvim_buf_set_option(buf, "modifiable", true)
  vim.api.nvim_buf_set_lines(buf, 0, -1, false, {
    "Running: " .. cmd,
    "=====================",
  })
  vim.api.nvim_buf_set_option(buf, "modifiable", false)

  local width = math.floor(vim.o.columns * 0.8)
  local height = math.min(20, vim.o.lines - 4)
  local row = math.floor((vim.o.lines - height) / 2)
  local col = math.floor((vim.o.columns - width) / 2)
  state.win = vim.api.nvim_open_win(buf, true, {
    relative = "editor",
    width = width,
    height = height,
    row = row,
    col = col,
    style = "minimal",
    border = "rounded",
    focusable = true,
  })
  state.visible = true

  state.job_id = vim.fn.jobstart(cmd, {
    stdout_buffered = true,
    stderr_buffered = true,
    on_stdout = function(_, data)
      if not data or not vim.api.nvim_buf_is_valid(buf) then return end
      vim.schedule(function()
        vim.api.nvim_buf_set_option(buf, "modifiable", true)
        for _, line in ipairs(data) do
          if line ~= "" then
            vim.api.nvim_buf_set_lines(buf, -1, -1, false, { line })
          end
        end
        vim.api.nvim_buf_set_option(buf, "modifiable", false)
      end)
    end,
    on_stderr = function(_, data)
      if not data or not vim.api.nvim_buf_is_valid(buf) then return end
      vim.schedule(function()
        vim.api.nvim_buf_set_option(buf, "modifiable", true)
        for _, line in ipairs(data) do
          if line ~= "" then
            vim.api.nvim_buf_set_lines(buf, -1, -1, false, { "ERROR: " .. line })
          end
        end
        vim.api.nvim_buf_set_option(buf, "modifiable", false)
      end)
    end,
    on_exit = function(_, code)
      if not vim.api.nvim_buf_is_valid(buf) then return end
      vim.schedule(function()
        vim.api.nvim_buf_set_option(buf, "modifiable", true)
        vim.api.nvim_buf_set_lines(buf, -1, -1, false, {
          "",
          "[Process exited with code " .. code .. "]",
          "[Press q to close | <leader>rr to toggle]",
        })
        vim.api.nvim_buf_set_option(buf, "modifiable", false)
      end)
    end,
  })

  local opts = { buffer = buf, silent = true }
  vim.keymap.set("n", "q", quit_process, opts)
  vim.keymap.set("n", "<C-Up>", function()
    if state.win and vim.api.nvim_win_is_valid(state.win) then
      vim.api.nvim_win_set_height(state.win, vim.api.nvim_win_get_height(state.win) - 1)
    end
  end, opts)
  vim.keymap.set("n", "<C-Down>", function()
    if state.win and vim.api.nvim_win_is_valid(state.win) then
      vim.api.nvim_win_set_height(state.win, vim.api.nvim_win_get_height(state.win) + 1)
    end
  end, opts)
  vim.keymap.set("n", "<C-Left>", function()
    if state.win and vim.api.nvim_win_is_valid(state.win) then
      vim.api.nvim_win_set_width(state.win, vim.api.nvim_win_get_width(state.win) - 2)
    end
  end, opts)
  vim.keymap.set("n", "<C-Right>", function()
    if state.win and vim.api.nvim_win_is_valid(state.win) then
      vim.api.nvim_win_set_width(state.win, vim.api.nvim_win_get_width(state.win) + 2)
    end
  end, opts)
end

vim.api.nvim_create_user_command("CompileRun", M.compile_and_run, {})
vim.api.nvim_create_user_command("CRQuit", quit_process, {})
vim.api.nvim_create_user_command("CRToggle", toggle_window, {})

return M

