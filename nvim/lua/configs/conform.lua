local M = {}

function M.setup()
  local conform = require("conform")

  conform.setup({
    formatters_by_ft = {
      css = { "prettier" },
      html = { "prettier" },
      json = { "prettier" },
      javascript = { "prettier" },
      javascriptreact = { "prettier" },
      typescript = { "prettier" },
      typescriptreact = { "prettier" },
      lua = { "stylua" },
      python = { "autopep8" },
      c = { "clangformat" },
      cpp = { "clangformat" },
      java = { "jdtls" },
      go = { "goimports", "gofmt" },
      yara = { "prettier" },
      yar = { "prettier" },
    },
    format_on_save = {
      lsp_format = "fallback",
      async = false,
      timeout_ms = 5000,
    },
    formatters = {
      autopep8 = {
        command = "autopep8",
        args = { "--stdin-filename", "$FILENAME", "-" },
      },
    },
    log_level = vim.log.levels.WARN,
  })

  vim.keymap.set({ "n", "v" }, "<leader>mp", function()
    conform.format({
      lsp_fallback = true,
      async = false,
      timeout_ms = 10000,
    })
  end, { desc = "Format file or range (in visual mode)" })
end

return M

